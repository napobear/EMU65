#include "../../include/ui/aim65controller.h"

Aim65Controller::Aim65Controller(std::shared_ptr<Aim65Proxy> aim65Proxy, QObject *parent) : QObject(parent)
{
    m_aimThread = std::unique_ptr<QThread>(new QThread);
    // Share the caller's existing shared_ptr instead of re-wrapping the raw
    // pointer, which would create a second, independent control block over
    // the same Aim65Proxy object and eventually double-delete it.
    m_aim65Proxy = aim65Proxy;
    m_aim65Proxy->moveToThread(m_aimThread.get());
    QObject::connect(m_aimThread.get(), SIGNAL(started()), m_aim65Proxy.get(), SLOT(Start()));
}

void Aim65Controller::Start()
{
    m_aimThread->start();
}
