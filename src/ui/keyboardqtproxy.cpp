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

char KeyboardProxy::GetPressedKey() const
{
    return this->m_pressedKey;
}

void KeyboardProxy::SetPressedKey(char key)
{
    this->m_pressedKey = key;
    this->m_keyboard->onKeyPressed(key);
}


