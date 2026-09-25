#include "../../include/ui/aim65proxy.h"

Aim65Proxy::Aim65Proxy(Aim65 *aim65, QObject *parent) : QObject(parent), UiProxy()
{
    this->m_aim65 = std::shared_ptr<Aim65>(Aim65::GetInstance());
    this->m_aim65->Initialise();
    // RegisterProxy() cannot be called here: shared_from_this() requires an
    // owning shared_ptr to already exist, which is only true once construction
    // (via std::make_shared at the call site) has completed. Callers must
    // invoke RegisterProxy() explicitly right after construction.
}

Aim65Proxy::~Aim65Proxy()
{
}

void Aim65Proxy::RegisterProxy()
{
    // Share the same control block as the caller's owning shared_ptr instead
    // of constructing an independent std::shared_ptr<Aim65Proxy>(this).
    UiProxyCollection::GetInstance()->InsertAim65Proxy(this->shared_from_this());
}

void Aim65Proxy::Start()
{
    // Without this, PC starts at 0 (the CPU struct is zero-initialized, not
    // loaded from the reset vector) instead of wherever 0xFFFC/0xFFFD point.
    // Address 0 holds opcode 0x00 (BRK), which vectors through 0xFFFE/0xFFFF
    // into the ROM's general IRQ/BRK handler -- code that assumes the real
    // reset routine already initialized its RAM-based indirect vectors
    // (e.g. it does JMP ($A404), uninitialized, so it jumps to 0 again).
    // The result is an infinite BRK loop that never reaches the real
    // power-on routine, so nothing is ever written to the LED display or
    // to the keyboard/printer registers.
    Cpu::GetInstance()->Reset();
    Cpu::GetInstance()->Run();
}

void Aim65Proxy::SetResetButton(bool newState)
{
    if (newState == true)
    {
        Cpu::GetInstance()->Reset();
        Start();
    }
    else
    {
        Cpu::GetInstance()->Halt();
    }

    this->m_aim65->SetPowerSwitch(newState);
}

bool Aim65Proxy::GetResetButton() const
{
    return true;
}
