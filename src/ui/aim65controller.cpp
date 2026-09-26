#include "../../include/ui/aim65controller.h"

Aim65Controller::Aim65Controller(std::shared_ptr<Aim65Proxy> aim65Proxy, QObject *parent) : QObject(parent)
{
    m_aimThread = std::unique_ptr<QThread>(new QThread);
    // Share the caller's existing shared_ptr instead of re-wrapping the raw
    // pointer, which would create a second, independent control block over
    // the same Aim65Proxy object and eventually double-delete it.
    m_aim65Proxy = aim65Proxy;
    m_aim65Proxy->moveToThread(m_aimThread.get());
    QObject::connect(m_aimThread.get(), SIGNAL(started()), m_aim65Proxy.get(), SLOT(PowerOn()));
}

bool Aim65Controller::GetPowerOn() const
{
    return m_powerOn;
}

bool Aim65Controller::GetStepMode() const
{
    return m_stepMode;
}

bool Aim65Controller::GetTtyMode() const
{
    return m_ttyMode;
}

void Aim65Controller::Start()
{
    if (m_aimThread->isRunning())
    {
        return;
    }

    m_powerOn = true;
    emit powerOnChanged();
    m_aimThread->start();
}

void Aim65Controller::Reset()
{
    if (m_powerOn)
    {
        Cpu::GetInstance()->RequestReset();
    }
}

void Aim65Controller::SetPowerOn(bool powerOn)
{
    if (powerOn == m_powerOn)
    {
        return;
    }

    m_powerOn = powerOn;

    // Aim65Proxy lives on the CPU thread, whose event loop is blocked for
    // as long as the interpreter runs. Halt() is atomic and makes Start()
    // return; the queued PowerOff()/PowerOn() then run on that thread in
    // order, never concurrently with the interpreter loop.
    Aim65Proxy *proxy = m_aim65Proxy.get();
    if (powerOn)
    {
        QMetaObject::invokeMethod(proxy, &Aim65Proxy::PowerOn, Qt::QueuedConnection);
    }
    else
    {
        Cpu::GetInstance()->Halt();
        QMetaObject::invokeMethod(proxy, &Aim65Proxy::PowerOff, Qt::QueuedConnection);
    }

    emit powerOnChanged();
}

void Aim65Controller::SetStepMode(bool stepMode)
{
    if (stepMode == m_stepMode)
    {
        return;
    }

    m_stepMode = stepMode;
    Cpu::GetInstance()->SetStepMode(stepMode);
    emit stepModeChanged();
}

void Aim65Controller::SetTtyMode(bool ttyMode)
{
    if (ttyMode == m_ttyMode)
    {
        return;
    }

    m_ttyMode = ttyMode;
    emit ttyModeChanged();
}
