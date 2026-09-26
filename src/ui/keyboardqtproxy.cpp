#include <QTimer>
#include "../../include/ui/keyboardqtproxy.h"

KeyboardProxy::KeyboardProxy(Keyboard *keyboard, QObject *parent) : QObject(parent), UiProxy()
{
    this->m_keyboard = std::shared_ptr<Keyboard>(keyboard);
    // RegisterProxy() cannot be called here: shared_from_this() requires an
    // owning shared_ptr to already exist, which is only true once construction
    // (via std::make_shared at the call site) has completed. Callers must
    // invoke RegisterProxy() explicitly right after construction.
}

KeyboardProxy::~KeyboardProxy()
{
}

void KeyboardProxy::RegisterProxy()
{
    // Share the same control block as the caller's owning shared_ptr instead
    // of constructing an independent std::shared_ptr<KeyboardProxy>(this),
    // which would double-manage (and eventually double-delete) this object.
    UiProxyCollection::GetInstance()->InsertKeyboardProxy(this->shared_from_this());
}

// The Monitor debounces by scanning, waiting and scanning again, and it is
// far quicker than a person: a tap shorter than this could be missed.
static const int MIN_KEY_HOLD_MS = 80;
// A modifier is held this long before its key: the Monitor looks at the
// modifiers at the start of its scan loop and at the key a moment later.
static const int MODIFIER_LEAD_MS = 100;

char KeyboardProxy::MapKey(int key, const QString &text, bool ctrl)
{
    // Qt::Key values for letters, digits and most punctuation are their
    // (upper case) ASCII codes; the text of Ctrl+letter is a control code.
    if (ctrl && key >= 0x20 && key <= 0x5F)
    {
        return static_cast<char>(key);
    }

    switch (key)
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return 0x0D;
    case Qt::Key_Backspace:
        return 0x08;
    case Qt::Key_Delete:
        return 0x7F;
    case Qt::Key_Escape:
        return 0x1B;
    // F1-F3 are the keys the keyboard produces '[', ']' and '^' with.
    case Qt::Key_F1:
        return '[';
    case Qt::Key_F2:
        return ']';
    case Qt::Key_F3:
        return '^';
    default:
        break;
    }

    // The AIM-65 has no lower case: type letters as upper case.
    const QString upper = text.toUpper();
    if (upper.isEmpty() || upper.at(0).unicode() > 0x7E)
    {
        return 0;
    }
    return static_cast<char>(upper.at(0).unicode());
}

void KeyboardProxy::keyDown(int key, const QString &text, bool ctrl)
{
    const char ch = MapKey(key, text, ctrl);
    Keyboard::KeyPress keyPress;
    if (ch == 0 || !Keyboard::FindKey(ch, ctrl, keyPress))
    {
        return;
    }

    this->m_clock.start();
    this->m_keyHeld = true;
    const unsigned pressId = ++this->m_pressId;

    if (!keyPress.shift && !keyPress.ctrl)
    {
        this->m_keyAppliedAt = 0;
        this->m_keyboard->PressKey(keyPress);
        return;
    }

    this->m_keyAppliedAt = MODIFIER_LEAD_MS;
    this->m_keyboard->PressModifiers(keyPress);
    QTimer::singleShot(MODIFIER_LEAD_MS, this, [this, pressId, keyPress]()
    {
        // Skip it if the key was let go and pressed again meanwhile.
        if (pressId == this->m_pressId)
        {
            this->m_keyboard->PressKey(keyPress);
        }
    });
}

void KeyboardProxy::keyUp()
{
    if (!this->m_keyHeld)
    {
        return;
    }
    this->m_keyHeld = false;

    // Long enough for the key to be seen once it has actually been pressed.
    const qint64 remaining = this->m_keyAppliedAt + MIN_KEY_HOLD_MS - this->m_clock.elapsed();
    if (remaining <= 0)
    {
        this->m_keyboard->ReleaseKey();
        return;
    }
    const unsigned pressId = this->m_pressId;
    QTimer::singleShot(static_cast<int>(remaining), this, [this, pressId]()
    {
        if (pressId == this->m_pressId && !this->m_keyHeld)
        {
            this->m_keyboard->ReleaseKey();
        }
    });
}
