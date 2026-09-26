#ifndef AIM65PROXY_H
#define AIM65PROXY_H

#include <QObject>
#include "../aim65.h"
#include "uiproxy.h"
#include "uiproxycollection.h"

class Aim65;

/**
 * This class decouples the application top-level window
 * Qt frontend from the entry point logic to the system.
 * Obtains and delivers all the necessary basic emulator functions that should be exposed
 * to the user.
 */
class Aim65Proxy : public QObject, public UiProxy, public std::enable_shared_from_this<Aim65Proxy>
{
    Q_OBJECT

public:
    explicit Aim65Proxy(Aim65 *aim65, QObject *parent = 0);
    virtual ~Aim65Proxy();
    virtual void RegisterProxy();
// These slots run the 6502 interpreter loop (Start() only returns once the
// CPU is halted), so they must only ever be invoked on the dedicated CPU
// thread this object is moved to -- via queued calls from Aim65Controller.
public slots:
    void Start();
    /**
     * Power switch ON: wipe RAM, then boot from the reset vector.
     */
    void PowerOn();
    /**
     * Power switch OFF, second half: runs after Cpu::Halt() has made
     * Start() return, and blanks the LED display.
     */
    void PowerOff();
signals:
private:
    std::shared_ptr<Aim65> m_aim65;
};

#endif /* AIM65PROXY_H */
