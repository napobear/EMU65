#ifndef KEYBOARDQTPROXY_H
#define KEYBOARDQTPROXY_H

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include "../iocomponents/keyboard.h"
#include "uiproxy.h"
#include "uiproxycollection.h"

/**
 * This class intercepts keyboard events from the user so they can be
 * processed by the emulator. It also limits the keyboard keys
 * to the the AIM-65 ASCII character set.
 */
class KeyboardProxy : public QObject, public UiProxy, public std::enable_shared_from_this<KeyboardProxy>
{
    Q_OBJECT
public:
    explicit KeyboardProxy(Keyboard *keyboard, QObject *parent = 0);
    virtual ~KeyboardProxy();
    virtual void RegisterProxy();
    /**
     * A PC key went down. The Monitor scans the keyboard matrix itself, so
     * this only records which AIM-65 key is held.
     * @param key Qt::Key value of the event.
     * @param text Text the event produced (layout aware), if any.
     * @param ctrl Whether CTRL is held; the key is then taken from `key`,
     *             since the text of Ctrl+letter is a control character.
     */
    Q_INVOKABLE void keyDown(int key, const QString &text, bool ctrl);
    /**
     * The PC key went up. The AIM-65 key is released once it has been held
     * long enough for the Monitor's debounce to see it.
     */
    Q_INVOKABLE void keyUp();
public slots:
signals:
private:
    std::shared_ptr<Keyboard> m_keyboard;
    QElapsedTimer m_clock;
    // Milliseconds after keyDown() at which the key itself is pressed.
    int m_keyAppliedAt = 0;
    bool m_keyHeld = false;
    // Lets a pending release notice the key was pressed again meanwhile.
    unsigned m_pressId = 0;
    /**
     * @return The AIM-65 character for a PC key, or 0 if it has no key.
     */
    static char MapKey(int key, const QString &text, bool ctrl);
};

#endif /* KEYBOARDQTPROXY_H */
