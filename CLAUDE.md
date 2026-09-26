# CLAUDE.md

Guida per lavorare su EMU65 (emulatore Rockwell AIM-65 basato su 6502, Qt/QML).
Progetto storico (~11 anni), attualmente basato su qmake e in fase di modernizzazione.

## Ambiente di sviluppo di riferimento
- openSUSE Tumbleweed
- KDE Plasma 6.7.5, KDE Frameworks 6.30.0
- Qt 6.11.2 (`qmake6` disponibile; moduli Qt6Core/Gui/Qml/Quick installati)
- Compilatore: g++ (SUSE) 16.2.0, `-std=c++17` usato per le verifiche di sintassi

## Regole apprese durante l'audit di sicurezza (da rispettare in modifiche future)

1. **Mai costruire un nuovo `std::shared_ptr<T>` a partire da un puntatore raw
   potenzialmente già gestito altrove** (incluso `this` dentro un metodo
   membro, es. `std::shared_ptr<T>(this)`). Crea un secondo control block
   indipendente sullo stesso oggetto e porta a un double-free quando entrambi
   gli shared_ptr arrivano a refcount zero. Pattern corretto: la classe eredita
   anche da `std::enable_shared_from_this<T>` e usa `shared_from_this()` —
   ma questo richiede che l'oggetto sia *già* posseduto da uno shared_ptr
   (quindi mai chiamabile dal costruttore: l'owning shared_ptr va creato dal
   chiamante con `std::make_shared`, e solo dopo si invoca un metodo tipo
   `RegisterProxy()`). Se una classe riceve un oggetto già posseduto da uno
   shared_ptr altrove, va propagato quello stesso shared_ptr, mai ri-avvolto
   il puntatore raw sottostante.
   Esempio reale corretto in questa sessione: `KeyboardProxy`, `LedDisplayProxy`,
   `Aim65Proxy`, `LedDisplay`, `Aim65Controller`.

2. **Ogni funzione non-`void` deve avere un `return` su tutti i percorsi di
   controllo.** L'assenza è undefined behaviour in C++, non un errore
   "innocuo" — può manifestarsi come crash intermittenti difficili da
   riprodurre (visto in `Printer::GetRegisterValue()` e `IOBus::Read()`).

3. **Qualunque costruttore/funzione che riceva un buffer (`byte*`) insieme a
   un range di indirizzi deve anche ricevere e validare la dimensione reale
   del buffer** prima di indicizzarlo (`IOComponent`). Non assumere mai che
   combaci con il range di indirizzi richiesto — un file ROM troncato o
   sostituito porta altrimenti a un heap buffer over-read i cui byte finiscono
   eseguiti come codice 6502.

4. **Le eccezioni C++ che attraversano il loop dell'interprete 6502
   (`Run6502()`) girano su un `QThread` dedicato**: se non vengono catturate,
   raggiungono nessun event loop e causano `std::terminate()`/SIGABRT,
   terminando l'intera applicazione. Il punto di cattura corretto è
   `Cpu::Run()`. Qualunque nuovo codice che gira su quel thread deve seguire
   lo stesso principio (mai lasciare un'eccezione propagare fuori dal thread).

5. **Il logging di debug che scrive su file deve sempre essere racchiuso in
   `#ifdef EMU65_DEBUG` / `#endif`**, mai lasciato attivo incondizionatamente
   in una build di release (visto in `LedDisplay::SetRegister`/`OutputCursor`,
   che scrivevano `leddbglog.txt` ad ogni pressione di tasto).

6. **Il linter/clangd di questo ambiente è sistematicamente rotto**: non trova
   `<map>`, `<memory>`, `<exception>`, `QObject`, ecc. su qualunque file del
   progetto (non è una regressione delle modifiche). Non fidarsi dei suoi
   diagnostici. Verificare sempre le modifiche con una compilazione reale:
   ```
   g++ -std=c++17 -fsyntax-only -Iinclude \
       $(pkg-config --cflags Qt6Core Qt6Gui Qt6Qml Qt6Quick) <file>.cpp
   ```

7. **`Debug.c` / `ConDebug.c`** (console di debug testuale del core M6502 di
   Fayzullin) non sono referenziati in `EMU65.pro` e non venivano compilati:
   rimossi dal repository come codice morto.

8. **Uno struct C (POD) usato come membro di una classe C++ non viene
   zero-inizializzato di default.** `Cpu::Cpu()` non inizializzava mai
   `M6502 m_cpu`, e `Reset6502()` a sua volta non tocca tutti i campi
   (`IPeriod`, `IBackup`, `IAutoReset`, `TrapBadOps`, `Trap`, `Trace`,
   `User` restavano indeterminati). `IPeriod` in particolare alimenta
   `ICount`, controllato dal loop dell'interprete per decidere quando
   richiamare `Loop6502()`: con un valore casuale, il comportamento della
   CPU (e quindi i sintomi di crash) dipendeva da cosa si trovava prima
   sull'heap/stack a quell'indirizzo — un bug quasi impossibile da
   riprodurre in modo consistente finché non si isola con AddressSanitizer.
   Regola generale: value-initializzare sempre (`m_cpu()` nella init-list,
   o `= {}`) ogni struct C usato come membro.

9. **Un `QObject` le cui `Q_PROPERTY` sono lette da bindings QML sul thread
   GUI non può essere mutato con chiamate C++ dirette da un altro thread**
   (qui: il `QThread` dedicato alla CPU). Il meta-object system di Qt
   marshalla automaticamente solo attraverso connessioni signal/slot o
   `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` — una chiamata
   C++ diretta a un setter da un thread estraneo è una race condition vera
   e propria. In `AimInspector` questo causava un **heap use-after-free**
   (confermato con AddressSanitizer: `QString::operator=` sul thread CPU
   liberava il buffer che il motore QML stava leggendo sul thread GUI nello
   stesso istante). Ogni `Update*Status()` ora marshalla la scrittura
   effettiva della proprietà con `QMetaObject::invokeMethod(this, [...]{},
   Qt::QueuedConnection)`.

10. **Per bug di memoria/concorrenza che si manifestano solo in modo
    intermittente o con sintomi "a distanza" (corruzione dell'heap rilevata
    in un allocatore non correlato al bug reale), non affidarsi al solo
    GDB**: ricompilare con
    `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g -O0"` e
    `-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"` individua la
    scrittura/lettura invalida esatta nel giro di un'esecuzione, invece di
    dover dedurla a ritroso da uno stack di corruzione. Il target
    "reale" per validare un fix di questo tipo è un avvio effettivo
    dell'app (`QT_QPA_PLATFORM=offscreen ./EMU65`, dato che l'ambiente non
    ha un display), non solo la compilazione.

11. **La coda di eventi del thread CPU è bloccata per tutto il tempo in
    cui gira l'interprete** (`Run6502()` è chiamato dentro uno slot): un
    `invokeMethod(..., Qt::QueuedConnection)` verso un `QObject` che vive
    lì (es. `Aim65Proxy`) viene eseguito solo quando `Run()` ritorna.
    Vale per power off/on (`Cpu::Halt()` è atomico e fa uscire il loop,
    poi gli slot in coda partono in ordine) ma **non** per dati da
    condividere in tempo reale con la CPU: reset e STEP usano flag
    atomici letti da `Cpu::CheckInterrupts()`. Dati scritti dalla GUI e
    letti dalla CPU (tastiera) vanno protetti con un mutex locale al
    componente, non marshallati sul thread CPU.

12. **Power cycle: azzerare lo stato che vive fuori dalla RAM.** Un tasto
    premuto a macchina spenta lasciava la linea IRQ alta
    (`Keyboard::onKeyPressed` la alza anche a CPU ferma); al Power ON
    l'IRQ pendente veniva servito con la RAM azzerata (vettori a zero) e
    il boot finiva nel loop BRK→IRQ→`JMP($A404)`→0 (`PC:0002` nel
    debugger). `Aim65::ClearVolatileMemory()` ora abbassa la linea IRQ e
    `main.qml` scarta i tasti finché `aim65Controller.powerOn` è falso.
    Ogni nuovo stato di periferica va azzerato lì.

13. **ThreadSanitizer è il complemento di ASan per le race** (ASan non le
    vede): `-DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1"` con
    `-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"`, in una cartella di
    build a parte (`build-tsan`, già ignorata da git). Le librerie Qt non
    sono instrumentate: le segnalazioni su `AimInspector::QueueStatusUpdate`
    (`aiminspector.cpp:47/51`, blocco heap del functor passato a
    `invokeMethod`) e quelle dentro Qt/Mesa/dbus sono falsi positivi noti.
    Con `TSAN_OPTIONS="log_path=..."` i report vanno su file.

## Workflow concordato con l'utente per questo tipo di lavoro
- Fix di sicurezza e correzioni di bug vanno su un branch dedicato e
  committati separatamente dalla modernizzazione del build system.
- La modernizzazione del build (qmake → CMake, porting Qt6) va su un
  ulteriore branch dedicato, creato solo dopo aver committato i fix di
  sicurezza.
- Questo file (`CLAUDE.md`) e `CHAT.md` sono tracciati in git (in
  precedenza erano esclusi via `.gitignore`).

## Modernizzazione build — fatto (branch `modernize/cmake-qt6`)
- [x] Migrato da qmake (`EMU65.pro`, rimosso) a CMake + Qt6
  (`CMakeLists.txt`, componenti Core/Gui/Qml/Quick/QuickControls2).
- [x] Rimosso lo shim legacy `qtquick2applicationviewer` (pensato per
  Symbian/Harmattan/Qt5, mai istanziato: `main.cpp` usa già
  `QQmlApplicationEngine` direttamente).
- [x] Rimossi dal controllo versione gli artefatti di build tracciati
  (`build/`, `debug/`, `*.o`, `*.obj`, `*.exe`, `*.pdb`, `*.ilk`,
  `Makefile*` generati, `EMU65.pro.user*`) e aggiunto `.gitignore` per
  build CMake, artefatti generici ed editor.
- [x] Rimossi i file spuri tracciati: `somefile.BIN`, `vc110.pdb`,
  `include/iocomponents/printer.h~`, `res/rom/#AIMMON11.OBJ#`,
  `dbgoutput.txt`, `leddbglog.txt`, `LED Tracing.txt` (log generati da
  run precedenti, finiti in git per errore).
- [x] Alzato `CONFIG += c++14` → `CMAKE_CXX_STANDARD 17`; rimosso l'uso di
  `register` come storage class specifier in `M6502.h`/`M6502.cpp`
  (rimosso dalla grammatica C++17, con GCC prima era solo un warning).
- [x] Portato `qml/EMU65/main.qml` e `aiminspector.qml` da
  `QtQuick.Controls 1.0` (rimosso in Qt6, l'app non si sarebbe nemmeno
  potuta avviare) a `QtQuick.Controls` (2.x) con import non versionati.
- [x] **Build verificata end-to-end**: configurazione (`cmake -S . -B build
  -G Ninja -DCMAKE_PREFIX_PATH=/usr/lib64/qt6`), compilazione pulita,
  avvio reale (`QT_QPA_PLATFORM=offscreen ./EMU65`, l'ambiente non ha un
  display) verificato attivo per 18+ secondi senza crash, sia in build
  normale sia sotto AddressSanitizer/UBSan.
- [x] Durante questa verifica sono emersi (e sono stati corretti) due bug
  di memoria/concorrenza reali, mai raggiungibili prima perché il
  processo moriva subito per l'eccezione non gestita della fase di
  sicurezza — vedi regole 8 e 9 sopra: struct `M6502` non inizializzata e
  race condition cross-thread su `AimInspector`.
- [x] **Copia di `qml/` e `res/` nella cartella di build** (PR #11): prima
  era un passo `POST_BUILD` del target `EMU65`, eseguito solo quando il
  binario veniva ricollegato — modificando solo un `.qml`, `cmake --build`
  non faceva nulla e l'app caricava la copia vecchia. Ora è un target a sé
  (`EMU65_runtime_files`) che dipende da tutti i file sotto `qml/` e `res/`
  (glob con `CONFIGURE_DEPENDS`, tracciato da `runtime_files.stamp`).
  Verificato: rebuild senza modifiche → `no work to do`; modifica di un
  solo `.qml` → ricopia senza relink.

## Finestra del debugger (`aiminspector.qml`) — note
- I pannelli LED/Keyboard/Printer Registers stanno in un proprio
  `ScrollView` che divide l'altezza a metà con "Memory Contents" (PR #10).
  Prima erano in un `RowLayout` senza limite di altezza: dopo il fix del
  reset il pannello LED mostra tutto `0xAC00-0xAC43` (~70 righe) e
  spingeva "Memory Contents" fuori dalla finestra, facendolo sembrare
  sparito. Se si aggiungono pannelli, mantenere altezze limitate/scrollabili.
- Il dump "Memory Contents" **non scorre più continuamente** come prima di
  PR #8, ed è corretto così: quello scorrimento era il loop spurio
  BRK→IRQ che scriveva di continuo nello stack. Dopo un boot reale il
  monitor attende input e scrive in RAM solo occasionalmente; il pannello
  mostra l'ultima zona scritta (tipicamente lo stack, `01Ex-01Fx`).

## Pannello frontale (`aim65controller`, `main.qml`) — note
- `Aim65Controller` espone `powerOn`, `stepMode`, `ttyMode` al QML; RESET,
  le due leve e il menu Computer li usano. Verificato con xcb + screenshot
  (`xdotool`/`spectacle`) e sotto ASan/UBSan/TSan.
- **Reset**: `Cpu::RequestReset()` alza un flag atomico, servito da
  `Cpu::CheckInterrupts()` sul thread CPU (con `IPeriod` a 0 viene
  chiamato dopo ogni istruzione). RAM preservata (warm start del monitor).
- **Power OFF/ON**: `Halt()` + slot in coda `PowerOff()`/`PowerOn()`;
  ON azzera RAM e RIOT RAM del monitor e la linea IRQ (regola 12).
- **STEP**: NMI dopo ogni istruzione con `OpPC < 0xE000` (`M6502::OpPC`,
  campo aggiunto da EMU65). **Verificato lato emulatore** (traccia
  temporanea in `CheckInterrupts()`, programma in RAM a `0x0200`, `$A402`
  puntato a un `RTI` della ROM): un NMI per istruzione utente, ritorno
  all'indirizzo giusto, registri e memoria corretti, le istruzioni ROM non
  generano NMI. **Non verificato**: come il Monitor riprende dopo l'NMI
  (comando dalla tastiera); ora che la tastiera è a matrice si può provare.
- **KB/TTY**: solo la posizione della leva, l'interfaccia TTY non è
  emulata.

## Tastiera (`Keyboard`, `KeyboardProxy`) — matrice 8x8
Il Monitor NON legge ASCII: scandisce una matrice. Scrive su `A480` (DRA2)
uno zero rotante (`7F,BF,DF,EF,F7,FB,FD,FE`, una colonna per volta, attivo
basso) e legge le righe da `A482` (DRB2): bit basso = tasto premuto, `FF` a
riposo. Il carattere è `KEY_TABLE[riga*8 + colonna]`, la tabella ROM a
`0xF421` (copiata in `keyboard.cpp`). Riga 0, colonne 4-6 sono i modificatori
(4 = CTRL, 5/6 = SHIFT); lo SHIFT lo applica la ROM stessa al carattere base
(`0xECA4-0xECBF`), quindi `Keyboard::FindKey` inverte anche quella logica.
- `Keyboard::GetRegisterValue(DRB2)` calcola le righe dallo strobe corrente in
  `A480` e dal tasto premuto; il valore scritto in `A482` non conta (su un
  6532 con DDRB=0 la lettura riflette i pin). Il costruttore azzerava
  `A482` e la ROM lo riazzerava all'init (`STA $A480,X` a `0xE0D7`): la
  routine `0xECEF` aspetta `A482 == FF` prima di scandire, quindi il boot
  restava fermo a `PC:EC42` (bug esistente da sempre, invisibile finché la
  tastiera scriveva ASCII).
- Nessun IRQ da tastiera: il Monitor fa polling.
- `KeyboardProxy::keyDown(key, text)`/`keyUp()` (QML `Keys.onPressed/
  onReleased`, autorepeat ignorato) mappano Return→0D, Backspace→08,
  Delete→7F, Esc→1B, F1-F3→`[` `]` `^`, altrimenti `text` in maiuscolo. Il
  rilascio è ritardato a un minimo di 40 ms perché il debounce della ROM
  (scan, attesa, riscansione) non perda un tap veloce.
- Verifica: traccia temporanea (mai committata) sul `RTS` a `0xECEB` che
  stampa `A`: `a→41`, `1→31`, `Shift+1→21`, Return→`0D`, Backspace→`08`,
  `F1→5B`, `.`→`2E` ecc. CTRL non è mappato. Lo `Shift+,` su layout
  italiano dà `;` perché è già il carattere prodotto dal PC.

## Prossimi passi
- Valutare `qt_add_qml_module`/risorse Qt embedded al posto della copia
  di `qml/`+`res/` accanto al binario (target `EMU65_runtime_files`), se
  si vuole un pacchetto singolo deployabile.
- `EMU65_harmattan.desktop` è specifico per Nokia Harmattan (piattaforma
  dismessa da oltre un decennio): valutare se rimuoverlo o tenerlo solo
  come nota storica.
- Aggiungere una vera CI (build + eventualmente un run smoke-test headless
  come quello fatto manualmente in questa sessione) per intercettare
  regressioni di questo tipo automaticamente.
- Vedi anche "Problemi noti non di sicurezza" sotto, ancora aperti.

## Problemi noti non di sicurezza, non ancora corretti

- **Il display LED principale dell'AIM-65 (nella finestra "EMU65", non nel
  debugger) resta visivamente vuoto**, anche se i registri LED (`AC00`
  ecc.) vengono scritti correttamente durante il boot (visibili nel
  pannello "LED Registers" del debugger). La causa profonda del boot
  bloccato è stata trovata e corretta (vedi sotto: mancava la chiamata a
  `Cpu::Reset()`); questo è un problema **diverso e più piccolo**, ancora
  aperto: `LedDisplay::SetRegister()` aggiorna il testo del display solo
  quando rileva una combinazione valida di bit W/CE su `RA_ADDR`
  (`GetTargetDisplayFromAddrRegister() != 5`), e durante il loop di boot
  osservato i registri `AC00-AC03` vengono scritti una volta sola con un
  pattern (`FF,04,BC,04`) che non sembra soddisfare quella condizione, poi
  mai più aggiornati finché il monitor non ha qualcosa di nuovo da
  mostrare (es. l'eco di un tasto premuto). Prima cosa da verificare in
  un prossimo giro: premere tasti reali nella finestra principale e
  controllare se **dopo** l'interazione il display si popola (il canale
  IRQ tastiera è stato verificato funzionante: un tasto premuto aggiorna
  correttamente `A482` nel pannello Keyboard Registers). Se anche dopo
  input da tastiera il display resta vuoto, sospettare la catena
  `LedDisplay::SetRegister → LedDisplayProxy::triggerDisplayDigitChanged
  → emit displayDigitChanged()`, che — come `AimInspector` prima del suo
  fix — viene chiamata direttamente dal thread CPU su un `QObject` letto
  da bindings QML sul thread GUI, senza `QMetaObject::invokeMethod(...,
  Qt::QueuedConnection)`: stesso pattern di race condition già corretto
  altrove in questa sessione, qui non ancora applicato.
  **Aggiornamento**: la race (`LedDisplayProxy::m_ledDisplays` scritto dal
  thread CPU e letto da `GetLedDisplay()` sul thread GUI, emissione diretta
  del segnale) è stata corretta con un mutex e `invokeMethod` in coda,
  confermata da TSan. **Non è stato verificato se il display principale
  ora si popola**: se resta vuoto la causa è un'altra, e il sospetto
  passa alla condizione W/CE di `SetRegister()` descritta sopra.
  **Aggiornamento 2** (tastiera a matrice): il boot ora arriva al ciclo di
  scansione e i registri LED (`AC00-AC03`) cambiano dopo i tasti (es. `AC02`
  da `BC` a `DE`), ma **il display principale resta comunque vuoto**. La
  causa non era la tastiera; si può ora indagare `LedDisplay::SetRegister`
  con il monitor che risponde davvero ai tasti.

- **[RISOLTO in una sessione precedente di questo task, poi corretto per
  davvero]** L'ipotesi iniziale ("la CPU resta bloccata indefinitamente
  in polling su `0xA405`, timer RIOT non emulato") era **sbagliata**: la
  causa reale, confermata tracciando le istruzioni eseguite dal reset
  byte per byte, era che **`Aim65Proxy::Start()` non chiamava mai
  `Cpu::Reset()`** prima di `Cpu::Run()` (la riga era presente ma
  commentata). Poiché `M6502 m_cpu` è zero-inizializzato (fix di una
  sessione precedente) ma il Program Counter non viene mai caricato dal
  vettore di reset (`0xFFFC-0xFFFD`), la CPU iniziava semplicemente
  all'indirizzo 0, dove il byte è 0x00 (BRK). Questo BRK spurio vettorizza
  attraverso `0xFFFE-0xFFFF` nel gestore IRQ/BRK generico della ROM
  (`0xE078`), che assume che la vera routine di reset abbia già
  inizializzato i suoi vettori indiretti in RAM: esegue `JMP ($A404)`, e
  poiché `0xA404-0xA405` sono ancora a zero, salta di nuovo a 0,
  richiudendo un loop BRK→IRQ→JMP($A404)→0 all'infinito, senza mai
  raggiungere la vera routine di power-on. Aggiunta la chiamata a
  `Cpu::Reset()` in `Aim65Proxy::Start()`: dopo il fix la CPU carica
  correttamente il PC da `0xE0BF` (bytes ROM a `0xFFFC-0xFFFD`), il boot
  procede, e tutti e tre i pannelli LED/Keyboard/Printer Registers del
  debugger mostrano dati reali; l'IRQ da tastiera è stato verificato
  funzionante end-to-end (pressione di tasto reale → `A482` si aggiorna
  col codice ASCII corretto). Metodo usato per la diagnosi: aggiungere
  temporaneamente un log su `Cpu::Read()` per le prime ~300 letture dal
  reset (mai tramite `gdb`, bloccato da `yama/ptrace_scope=1` in questo
  ambiente), poi decodificare a mano i byte della ROM (`xxd`) alle
  posizioni coinvolte per identificare l'istruzione reale.
- **Prestazioni**: `IOComponent::DumpMemory(word)` chiamava
  `GetAddressRange()` (che restituiva `std::vector<word>` *per valore*)
  fino a ~66 volte per singola invocazione dentro il corpo del ciclo —
  per la RAM (1024 registri) questo significava centinaia di copie
  dell'intero vettore per ogni singolo byte scritto dalla CPU, rendendo
  l'interprete 6502 ordini di grandezza più lento del dovuto (verificato:
  l'avanzamento osservato nel pannello "Memory Contents" è passato da
  ~2 byte/secondo a >10 byte/secondo dopo il fix). Corretto: 
  `GetAddressRange()` ora ritorna `const std::vector<word>&`, e
  `DumpMemory()` legge min/max una sola volta prima del ciclo invece di
  richiamarla ad ogni iterazione. Se in futuro l'interprete risultasse
  ancora lento, `IOComponent::m_registers` (uno `std::map<word,byte>`)
  è un altro candidato: un array piatto indicizzato da `address - minAddress`
  sarebbe O(1) invece di O(log n) per ogni accesso a registro.

- `FilePrinter::DEFAULT_PRINTING_PATH` (`"../../res/printer.txt"`) è un
  percorso relativo hardcoded, risolto rispetto alla working directory del
  processo, non alla posizione dell'eseguibile: fragile se l'app viene
  lanciata da una directory diversa.
- `Aim65Proxy::SetResetButton()` non implementa un reset "vero" delle
  periferiche (solo `Cpu::Reset()` + riavvio del loop); da rivedere se si
  vuole un power-cycle realistico dell'AIM-65 emulato.
