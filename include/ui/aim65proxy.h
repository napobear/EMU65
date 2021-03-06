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
class Aim65Proxy : public QObject, public UiProxy
{
    Q_OBJECT
    Q_PROPERTY(bool resetButton
               READ GetResetButton
               WRITE SetResetButton)

public:
    explicit Aim65Proxy(Aim65 *aim65, QObject *parent = 0);
    virtual ~Aim65Proxy();
    virtual void RegisterProxy();
    bool GetResetButton() const;
public slots:
    void SetResetButton(bool newState);
    void Start();
signals:
private:
    std::shared_ptr<Aim65> m_aim65;
};

#endif /* AIM65PROXY_H */
