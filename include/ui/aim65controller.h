#ifndef AIM65CONTROLLER_H
#define AIM65CONTROLLER_H

#include <memory>
#include <QObject>
#include <QThread>
#include "../include/ui/aim65proxy.h"

/**
 * Front panel of the emulated AIM-65, exposed to QML. Lives on the GUI
 * thread and holds the switch positions QML binds to; the emulator itself
 * runs on m_aimThread and is only ever driven from here through queued
 * calls or thread-safe (atomic) Cpu methods.
 */
class Aim65Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool powerOn READ GetPowerOn WRITE SetPowerOn NOTIFY powerOnChanged)
    Q_PROPERTY(bool stepMode READ GetStepMode WRITE SetStepMode NOTIFY stepModeChanged)
    Q_PROPERTY(bool ttyMode READ GetTtyMode WRITE SetTtyMode NOTIFY ttyModeChanged)
public:
    Aim65Controller(std::shared_ptr<Aim65Proxy> aim65, QObject *parent = 0);
    bool GetPowerOn() const;
    bool GetStepMode() const;
    bool GetTtyMode() const;
public slots:
    /**
     * Starts the CPU thread and powers the machine on. Call once, at startup.
     */
    void Start();
    /**
     * RESET button: restarts the 6502 from the reset vector without wiping
     * RAM. Ignored while powered off.
     */
    void Reset();
    void SetPowerOn(bool powerOn);
    /**
     * RUN/STEP switch: true = STEP.
     */
    void SetStepMode(bool stepMode);
    /**
     * KB/TTY switch: true = TTY. Only the switch position is tracked; the
     * TTY interface itself is not emulated.
     */
    void SetTtyMode(bool ttyMode);
signals:
    void powerOnChanged();
    void stepModeChanged();
    void ttyModeChanged();
private:
    std::unique_ptr<QThread> m_aimThread;
    std::shared_ptr<Aim65Proxy> m_aim65Proxy;
    bool m_powerOn = false;
    bool m_stepMode = false;
    bool m_ttyMode = false;
};

#endif /* AIM65CONTROLLER_H */
