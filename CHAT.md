# CHAT.md — Log di sessione EMU65

Riepilogo della sessione di lavoro su EMU65 (emulatore Qt/QML del Rockwell
AIM-65 a 6502). Serve a riprendere il lavoro domani senza dover rileggere
l'intera conversazione. Non è un transcript letterale, ma una sintesi
completa e in ordine cronologico di cosa è stato fatto, trovato e deciso.

Vedi anche `CLAUDE.md` (regole tecniche apprese, stato del progetto,
problemi noti) — i due file sono complementari: `CLAUDE.md` è la
"documentazione tecnica", `CHAT.md` è il "diario di sessione". Entrambi
sono tracciati in git.

**Stato repo a fine sessione**: branch `master`, ultimo commit `d18bc8e`
(merge PR #11). Tutte le PR della giornata (#1-#11) sono state mergiate in
`master` e i branch corrispondenti eliminati sia in locale che su
`origin`. Le modifiche a `CLAUDE.md`/`CHAT.md` con il riepilogo di §8
potrebbero essere ancora da committare (controllare `git status`). Resta su `origin` un branch preesistente `qt6` (non
creato in questa sessione, mai toccato — vecchio tentativo di porting,
fermo al commit `0d9540f`, precedente a tutto il lavoro di oggi).


## 0. Richiesta iniziale

L'utente ha chiesto di modernizzare questo progetto 11enne (allora su
qmake/Qt5-era) e di trovare vulnerabilità di sicurezza. Ambiente di
riferimento: openSUSE Tumbleweed, KDE Plasma 6.7.5, KDE Frameworks 6.30.0,
Qt 6.11.2.


## 1. Audit di sicurezza — PR #1 (`security/crash-and-memory-fixes`)

Prima fase concordata con l'utente: fix di sicurezza/crash prima, poi
modernizzazione del build. Bug trovati e corretti:

1. **Crash per eccezione non gestita su thread Qt** (causa più probabile
   dei SIGABRT/SIGSEGV nei commit storici del progetto): `IOBus::GetIOChannel()`
   lancia `UnmappedMemoryException` per accessi a memoria non mappata,
   sollevata nel cuore del loop dell'interprete 6502 (`Run6502()`, su un
   `QThread` dedicato) e mai catturata. `Cpu::Run()` ora la cattura e ferma
   la CPU invece di far crashare tutto il processo.
2. **Heap buffer over-read nel caricamento ROM**: `IOComponent` leggeva
   sempre `maxAddress-minAddress+1` byte da un buffer senza validarne la
   dimensione reale. Aggiunta validazione esplicita con eccezione se il
   buffer è troppo piccolo.
3. **`ImageLoader`**: non gestiva errori `tellg()`/`seekg()` né letture
   troncate. Riscritto per riportare solo i byte effettivamente letti.
4. **`Printer::PrintBuffer()`**: leggeva un buffer di stack non
   inizializzato per decidere se riempirlo (possibile leak di stack nel
   file di stampa).
5. **UB per `return` mancante** in `Printer::GetRegisterValue()` e
   `IOBus::Read()`.
6. **Logging di debug incondizionato** in `LedDisplay::SetRegister`/
   `OutputCursor` (scriveva `leddbglog.txt` ad ogni tasto, anche in
   release) — racchiuso in `#ifdef EMU65_DEBUG`.
7. **Double-free reale** (non solo teorico): pattern `std::shared_ptr<T>(this)`
   in `KeyboardProxy`/`LedDisplayProxy`/`Aim65Proxy`/`LedDisplay`/
   `Aim65Controller` sostituito con `enable_shared_from_this` +
   `shared_from_this()`, chiamato solo dopo che l'oggetto è posseduto da
   uno `shared_ptr` esterno (mai dal costruttore).
8. Ricollegato `Aim65Controller` (era codice morto commentato) e sistemato
   `main.qml` che referenziava una context property QML inesistente.
9. Rimossi `Debug.c`/`ConDebug.c`, mai compilati (non in `EMU65.pro`).

Bug aggiuntivi minori: `AimInspector::SetPrinterStatus` scriveva nel
membro sbagliato (`m_ledStatus` invece di `m_printerStatus`).


## 2. Modernizzazione build — PR #2 (`modernize/cmake-qt6`)

- Migrato da qmake (`EMU65.pro`, rimosso) a **CMake + Qt6**
  (`CMakeLists.txt`: Core/Gui/Qml/Quick/QuickControls2, C++17, AUTOMOC).
- Rimosso lo shim `qtquick2applicationviewer` (mai istanziato, era boilerplate
  Qt Creator per Symbian/Harmattan/Qt5).
- Ripulito il repo da artefatti di build tracciati per errore (`build/`,
  `debug/`, `*.o/.obj/.exe/.pdb/.ilk`, Makefile generati, `EMU65.pro.user*`,
  `vc110.pdb`, `somefile.BIN`, file spuri, log di run vecchi). Aggiunto
  `.gitignore`.
- Rimosso `register` come storage class specifier (`M6502.h`/`.cpp`) —
  rimosso dalla grammatica C++17.
- Portato `main.qml`/`aiminspector.qml` da `QtQuick.Controls 1.0` (**non
  esiste più in Qt6** — senza questo fix l'app non si sarebbe nemmeno
  potuta avviare) a `QtQuick.Controls` 2.x.
- **Durante la prima build/run reale** sono emersi (e sono stati corretti
  nello stesso PR) due bug di memoria mai raggiungibili prima perché il
  processo moriva subito per il bug §1.1 sopra:
  - `Cpu::Cpu()` non inizializzava `M6502 m_cpu` (struct C POD) →
    comportamento indeterminato dipendente da garbage residuo.
  - Race condition cross-thread reale in `AimInspector` (scritture dirette
    a `QString` `Q_PROPERTY` dal thread CPU, lette dal thread GUI) → **heap
    use-after-free confermato con AddressSanitizer**. Fix: marshalling con
    `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.


## 3. README — PR #3 (`docs/readme-build-instructions`)

Riscritto interamente (era in inglese ma obsoleto/con mojibake, layout
qmake-era): struttura del progetto aggiornata, istruzioni di build CMake
verificate eseguendole letteralmente, sezione licenza.


## 4. Bug scoperti solo eseguendo l'app sul desktop reale

Fin qui l'app non era mai stata avviata con un vero display. Da qui in poi
il lavoro è stato: build → avvio su `DISPLAY=:1` (sessione desktop reale
dell'utente, non headless) → screenshot/interazione con `xdotool`/
`spectacle` per verificare visivamente ogni fix. **`gdb`/`ptrace` sono
bloccati in questo ambiente** (`yama/ptrace_scope=1`) — mai disponibili per
debug interattivo; tutta la diagnostica è stata fatta con logging
temporaneo (mai committato) rimosso a fine indagine.

### PR #4 (`fix/qml-load-errors`) — la UI non si caricava affatto
- `main.qml`: `qsTr("colorname")` assegnato a proprietà `color` — Qt6 lo
  rifiuta (`Invalid property assignment: color expected`), un solo errore
  del genere fa fallire il caricamento dell'intero componente. Sostituito
  con letterali stringa.
- `main.qml`/`aiminspector.qml`: sintassi `Connections { onFoo: {...} }`
  deprecata/non funzionante in Qt6 → convertita a
  `function onFoo() {...}`.
- `main.cpp`: `QQmlApplicationEngine` costruito con il path del QML (carica
  subito, nel costruttore) **prima** di impostare le context properties
  (`aim65`, `keyboard`, `ledDisplay`, `aimInspector`...) → ogni binding le
  vedeva indefinite. Risolto separando costruzione, `setContextProperty`,
  poi `load()`.

### PR #5 (`fix/gui-unresponsive-debug-flood`) — click che non rispondevano
`IOComponent::SetRegister()` chiama `UpdateDebugStatus()` su **ogni singola
scrittura di memoria**, RAM inclusa. Con la CPU che gira senza limiti di
velocità, questo inondava la coda eventi del thread GUI (via
`AimInspector`) a ritmo di CPU, affamando l'input utente. Fix reale:
**coalescing** in `AimInspector` (al massimo un aggiornamento "in volo" per
tipo di stato, sempre col valore più recente) invece di disabilitare la
funzione. Verificato con click reali (`xdotool`): menu Computer → Exit,
processo terminato correttamente.

### PR #6 (`fix/debugger-layout-and-live-updates`)
Conseguenza del fix precedente: avevo temporaneamente gated
`IOComponent`/`LedDisplay::UpdateDebugStatus()` dietro `#ifdef EMU65_DEBUG`
(mai definito) per fermare il flood — ma la finestra debugger è sempre
attiva, non è una feature di debug opzionale. Sostituito con il coalescing
sopra; rimossi i gate. Riscritto il layout di `aiminspector.qml` con
`QtQuick.Layouts` (era tutto sovrapposto per via di anchor sbagliati).

### PR #7 (`fix/debugger-contrast-and-status-panels`)
- Contrasto: colori espliciti (sfondo scuro, testo chiaro) invece di
  ereditare il tema di sistema (KDE dark rendeva il testo illeggibile).
- `Keyboard::UpdateDebugStatus()`/`Printer::UpdateDebugStatus()` erano
  **stub vuoti**, mai collegati ad `AimInspector` — implementati.
- "CPU Status:" era bloccato dietro `CPU_DEBUG`, macro mai definita in
  nessuna parte del progetto — impossibile che mostrasse qualcosa. Spostato
  `GetCpuDebugData()` fuori dal guard `EMU65_DEBUG`, chiamato
  incondizionatamente da `Cpu::Write()`.
- **Bug di performance reale trovato durante l'indagine**: `DumpMemory()`
  chiamava `GetAddressRange()` (`std::vector<word>` per valore, non per
  riferimento) fino a ~66 volte per invocazione, dentro un ciclo — per la
  RAM (1024 registri) centinaia di copie del vettore per ogni singolo
  byte scritto. Misurato un miglioramento di 5x+ dopo il fix
  (`GetAddressRange()` ora ritorna `const&`, bounds letti una volta sola).

### PR #8 (`fix/cpu-reset-vector-not-loaded`) — la vera causa del boot bloccato
Il pannello CPU Status ora funzionava, ma LED/Keyboard/Printer Registers
restavano vuoti e il display principale non mostrava nulla. **Ipotesi
iniziale sbagliata**: pensavo la CPU fosse bloccata in polling su
`0xA405` aspettando un timer RIOT (R6532) mai emulato — sembrava plausibile
dai pattern osservati (stack che oscillava in 0x0100-0x01FF) ma era una
falsa pista.

**Causa reale**, trovata tracciando le istruzioni eseguite dal reset byte
per byte (log temporaneo su `Cpu::Read()`, mai committato):
`Aim65Proxy::Start()` aveva la chiamata a `Cpu::GetInstance()->Reset()`
**presente ma commentata**. La CPU partiva quindi da PC=0 (mai caricato
dal vettore di reset) invece che dal vero entry point. L'indirizzo 0
contiene 0x00 (BRK), che vettorizza attraverso 0xFFFE/0xFFFF nel gestore
IRQ/BRK generico della ROM (0xE078) — il quale assume che la vera routine
di reset abbia già inizializzato i suoi vettori indiretti in RAM ed
esegue `JMP ($A404)`; siccome 0xA404-0xA405 sono ancora zero, il salto
torna a 0, richiudendo un loop BRK→IRQ→JMP($A404)→0 **all'infinito**,
senza mai raggiungere la vera routine di power-on a 0xE0BF (il reset
vector reale, letto da 0xFFFC/0xFFFD).

Fix: scommentata la chiamata a `Reset()`. **Verificato**: PC ora parte
correttamente da 0xE0BF, il boot procede, tutti e tre i pannelli
LED/Keyboard/Printer Registers si popolano con dati reali dal monitor ROM
in esecuzione, e l'IRQ da tastiera funziona end-to-end (tasto premuto →
`A482` si aggiorna subito col codice ASCII corretto, CPU riprende
correttamente dopo l'interrupt).


## 5. Problema aperto, non ancora risolto

**Il display LED visivo nella finestra principale ("EMU65") resta vuoto**,
anche se i registri LED (`AC00` ecc.) vengono scritti correttamente durante
il boot (visibile nel pannello "LED Registers" del debugger — quindi NON è
più il bug del reset vector, quello è risolto).

Ipotesi da verificare per prima domani (nell'ordine):

1. **Provare input da tastiera reale** nella finestra principale e
   controllare se il display si popola *dopo* l'interazione — il canale
   IRQ tastiera è verificato funzionante end-to-end, ma non ho ancora
   controllato se un'interazione utente sblocca anche l'aggiornamento del
   display.
2. Se anche dopo input da tastiera il display resta vuoto, il sospetto
   principale è: `LedDisplay::SetRegister()` aggiorna il testo solo se
   `GetTargetDisplayFromAddrRegister() != 5` (cioè un solo display
   selezionato con bit W attivo) — durante il boot osservato `AC00-AC03`
   vengono scritti una volta sola con un pattern (`FF,04,BC,04`) che
   potrebbe non soddisfare quella condizione, e poi mai più aggiornati.
3. Sospetto secondario, probabilmente il più concreto: la catena
   `LedDisplay::SetRegister → LedDisplayProxy::triggerDisplayDigitChanged
   → emit displayDigitChanged()` viene chiamata **direttamente dal thread
   CPU** su un `QObject` (`LedDisplayProxy`) le cui proprietà sono lette
   da QML sul thread GUI — **esattamente lo stesso pattern di race
   condition già trovato e corretto in `AimInspector`** (fix PR #2/#6 di
   questa sessione), ma **mai applicato a `LedDisplayProxy`**. Andrebbe
   marshallato con `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`
   allo stesso modo.

Metodo di lavoro consolidato per continuare: build → avvio con
`DISPLAY=:1 QT_QPA_PLATFORM=xcb ./build/EMU65` (necessario xcb, non
Wayland nativo, per poter usare `xdotool`/`spectacle`) → screenshot con
`spectacle -b -f -n -o <file>.png` → lettura screenshot con lo strumento
Read → per tracciare l'esecuzione reale, log temporaneo su `Cpu::Read()`/
`Cpu::Write()` (mai committare), rimosso a fine indagine con
`git checkout -- src/M6502/cpu.cpp`.


## 6. PR mergiate in questa sessione (riferimento rapido)

| PR | Branch | Titolo | Commit merge |
|----|--------|--------|--------------|
| #1 | `security/crash-and-memory-fixes` | Fix crash and memory-safety bugs found in a security audit | `0cdeb3c` |
| #2 | `modernize/cmake-qt6` | Migrate build to CMake/Qt6 and fix two memory bugs surfaced by it | `0a5db7d` |
| #3 | `docs/readme-build-instructions` | Rewrite README.txt with build instructions | `83988de` |
| #4 | `fix/qml-load-errors` | Fix three QML load failures that left the UI blank under Qt6 | `05b4f68` |
| #5 | `fix/gui-unresponsive-debug-flood` | Fix GUI becoming unresponsive to clicks during normal operation | `6c07d37` |
| #6 | `fix/debugger-layout-and-live-updates` | Fix debugger window layout and restore live memory scrolling | `c66fd2b` |
| #7 | `fix/debugger-contrast-and-status-panels` | Fix debugger contrast, wire up status panels, fix DumpMemory perf bug | `de66971` |
| #8 | `fix/cpu-reset-vector-not-loaded` | Fix the actual root cause of the boot hang: Cpu::Reset() was never called | `1f638bf` |
| #9 | `docs/claude-notes` | Track CLAUDE.md and CHAT.md in git | `1dfe05e` |
| #10 | `fix/debugger-memory-panel-hidden` | Keep the debugger's Memory Contents panel visible | `229f8cf` |
| #11 | `fix/cmake-copy-qml-on-change` | Copy qml/ and res/ to the build dir whenever they change | `d18bc8e` |

Tutte su `github.com/napobear/EMU65`, workflow ripetuto identico ogni
volta: branch dedicato → commit descrittivo → push → `gh pr create` →
`gh pr merge --merge` → sync locale di `master` → cancellazione branch
locale e remoto.


## 7. Note pratiche sull'ambiente (per non riscoprirle domani)

- `gh` (GitHub CLI) è installato e autenticato come `napobear` — il primo
  `which gh` in questa sessione aveva fallito per un problema di hashing
  della shell, non perché mancasse davvero.
- `gdb`/ptrace **non funzionano** verso processi non-figli
  (`yama/ptrace_scope=1`). Per ispezionare lo stato a runtime, unica via
  praticabile: logging temporaneo nel codice C++, mai committato.
- Il linter/clangd integrato in questo ambiente è **sistematicamente
  rotto** (non trova `<map>`, `<memory>`, `QObject`, ecc. su qualunque
  file). Ignorare i suoi diagnostici; verificare sempre con compilazione
  reale (`g++ -std=c++17 -fsyntax-only -Iinclude $(pkg-config --cflags
  Qt6Core Qt6Gui Qt6Qml Qt6Quick) <file>.cpp`, o build CMake completa).
- Per testare l'interazione utente serve `QT_QPA_PLATFORM=xcb` (non la
  piattaforma Wayland nativa, che `xdotool`/`spectacle` non possono
  pilotare) — l'ambiente ha sia `DISPLAY=:1` che `WAYLAND_DISPLAY`
  impostati, quindi Qt sceglierebbe Wayland di default.
- Attenzione ai processi di test lasciati in background tra un giro di
  debug e l'altro: controllare sempre `ps aux | grep -i emu65` prima di
  interpretare i risultati di un nuovo test (in questa sessione mi è
  capitato più volte di misurare/osservare un processo stantio invece di
  quello appena ricompilato).


## 8. Seconda parte della giornata (2026-09-26) — PR #9, #10, #11

### PR #9 (`docs/claude-notes`) — `CLAUDE.md` e `CHAT.md` in git
Su richiesta dell'utente i due file di note sono ora versionati: tolti da
`.gitignore` e aggiornate le frasi che dicevano il contrario.

### PR #10 (`fix/debugger-memory-panel-hidden`) — "Memory Contents" sparito
Segnalazione: nella finestra del debugger non si vedevano più scorrere le
locazioni di memoria. Screenshot della finestra (720x640): il pannello
c'era ancora, ma **fuori dalla finestra**. Dopo il fix del reset (PR #8)
il pannello "LED Registers" mostra l'intero range `0xAC00-0xAC43` (~70
righe) in un `RowLayout` senza limite di altezza, e spingeva "Memory
Contents" sotto il bordo. Fix: i tre pannelli registri in un proprio
`ScrollView`, con `Layout.preferredHeight: 1` sia su quello sia sullo
`ScrollView` della memoria (dividono l'altezza a metà). Verificato con
screenshot: entrambi visibili, memoria sullo stack `01E6-01F6`.

Nota importante: il dump **non scorre più di continuo** e non è un bug —
lo scorrimento di prima era il loop spurio BRK→IRQ (§4, PR #8) che
scriveva sullo stack senza sosta. Ora il monitor fa il boot e attende un
tasto.

### PR #11 (`fix/cmake-copy-qml-on-change`) — QML stantio nel build
Emerso durante la verifica della PR #10: dopo aver modificato
`aiminspector.qml`, `cmake --build build` rispondeva `no work to do` e
l'app caricava ancora il QML vecchio. La copia di `qml/`+`res/` era un
`POST_BUILD` del target `EMU65`, eseguito solo al relink. Ora è un target
separato `EMU65_runtime_files` con uno stamp file che dipende da tutti i
file sotto `qml/` e `res/` (`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`).
Verificato su una build pulita: nessuna modifica → nulla da fare;
modifica di un `.qml` → solo ricopia, niente relink; smoke test
`offscreen` vivo dopo 8 s.

### Stato del problema aperto (§5)
Invariato: il display LED della finestra principale resta vuoto. Oggi non
è stato indagato; le ipotesi di §5 restano il punto di partenza.


## 9. Pannello frontale, race e sanitizer (2026-09-26, branch `feature/front-panel-controls`)

Obiettivo: far funzionare RESET, power, RUN/STEP e KB/TTY. Ripreso da un
lavoro non committato (e non verificato) trovato nel working tree.

- **Verifica reale** con `QT_QPA_PLATFORM=xcb`: `xdotool` per i click,
  `spectacle -b -n -f -o file.png` per lo schermo intero (`import -window`
  su xcb restituiva immagini stantie o falliva sulla root), `magick` per
  ritagliare/affiancare. Il menu popup non compare nella cattura della
  sola finestra: serve lo schermo intero.
- **Bug trovato**: tasto premuto a macchina spenta → Power ON bloccato in
  `PC:0002` (loop BRK, regola 12 di CLAUDE.md). Causa: IRQ pendente della
  tastiera servito con la RAM appena azzerata. Fix: `ClearIRQLine()` in
  `ClearVolatileMemory()` + tasti scartati in QML a macchina spenta.
  Riprodotto prima, verificato dopo.
- **ASan/UBSan**: scenario completo + stress (raffiche di tasti, power
  toggle ravvicinati), nessun errore.
- **TSan** (`build-tsan`): 32 segnalazioni, di cui 5 vere nel codice del
  progetto — `LedDisplayProxy` (vettore LED), `Keyboard`
  (`DRB2`) e `IOBus::m_irqLine`. Corrette in un commit separato
  (mutex locale al componente, atomico, `invokeMethod` in coda). Restano
  solo le segnalazioni su `AimInspector::QueueStatusUpdate`, falsi
  positivi (Qt non instrumentato).
- **Cosa NON è stato verificato**: single-step reale in modalità STEP con
  un programma utente; se il display LED principale ora si popola.
- Commit: `0af2a37` (controlli), `d86651d` (race), più questo delle note.


## 10. Tastiera a matrice (2026-09-26, branch `feature/keyboard-matrix-scan`)

Partito da "far accettare Invio": `IsValidChar` (32-94) scartava 0x0D e
`main.qml` troncava `event.key` a `char`. Ma il Monitor non legge ASCII:
scandisce una matrice (strobe su `A480`, righe da `A482`, tabella ROM a
`0xF421`), quindi ho disassemblato la routine `0xEC38-0xED2C` (piccolo
disassemblatore Python nello scratchpad) e riscritto la tastiera.

- **Scoperta**: `A482` a riposo doveva valere `FF`; l'emulatore lo teneva a
  `00` (costruttore + `STA $A480,X` a `0xE0D7` nella ROM), e `0xECEF`
  aspetta `FF` prima di scandire: il boot restava fermo a `PC:EC42`.
  Provato forzando `DRB2=FF`: la CPU passa al ciclo di scansione.
- **Implementazione**: `Keyboard::PressKey/ReleaseKey`, lettura di `DRB2`
  calcolata dalla matrice, `KeyboardProxy::keyDown/keyUp` (autorepeat
  ignorato, rilascio minimo 40 ms). Nessun IRQ.
- **Verifica**: traccia temporanea sul `RTS` a `0xECEB` (mai committata):
  Invio→`0D`, `Shift+1`→`21`, F1→`5B` ecc. TSan: solo i falsi positivi noti.
  Regressione (tasto a macchina spenta + Power ON) ok.
- **Ambiente**: KDE ha chiesto il consenso per `xdotool` (controllo input);
  va concesso dall'utente, mai cliccato in autonomia.
- **Aperto**: il display principale resta vuoto (i registri LED cambiano);
  il resume di STEP via comandi del Monitor non è ancora provato.

