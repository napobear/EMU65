#include <QObject>
#include <QThread>
#include <QCoreApplication>
#include <QtGui/QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "../include/aim65.h"
#include "../include/ui/uiproxycollection.h"
#include "../include/debug/aiminspector.h"
#include "../include/ui/aim65proxy.h"
#include "../include/ui/aim65controller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Construct engines without a URL and load() them explicitly once every
    // context property they reference has been set. QQmlApplicationEngine's
    // (url)-taking constructor loads the component immediately, so setting
    // context properties afterwards (as this used to do) left aim65,
    // aim65Controller, keyboard, ledDisplay and aimInspector all undefined
    // for QML property bindings evaluated at load time.
    QQmlApplicationEngine mainWindow;
    QQmlApplicationEngine aimInspectorWindow;
    QObject::connect(&mainWindow, &QQmlApplicationEngine::objectCreationFailed,
                      &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    QObject::connect(&aimInspectorWindow, &QQmlApplicationEngine::objectCreationFailed,
                      &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    UiProxyCollection* proxyCollection = UiProxyCollection::GetInstance();

    // Construct via make_shared so an owning shared_ptr exists before
    // RegisterProxy() calls shared_from_this() on it, and so Aim65Controller
    // can share that same control block instead of re-wrapping a raw pointer.
    auto aim65Proxy = std::make_shared<Aim65Proxy>(Aim65::GetInstance());
    aim65Proxy->RegisterProxy();
    Aim65Controller* aim65Controller = new Aim65Controller(aim65Proxy);

    aimInspectorWindow.rootContext()->setContextProperty("aimInspector", AimInspector::GetInstance());
    mainWindow.rootContext()->setContextProperty("aim65", proxyCollection->GetAim65Proxy());
    mainWindow.rootContext()->setContextProperty("aim65Controller", aim65Controller);
    mainWindow.rootContext()->setContextProperty("keyboard", proxyCollection->GetKeyboardProxy());
    mainWindow.rootContext()->setContextProperty("ledDisplay", proxyCollection->GetLedDisplayProxy());

    mainWindow.load(QStringLiteral("qml/EMU65/main.qml"));
    aimInspectorWindow.load(QStringLiteral("qml/EMU65/aiminspector.qml"));

    aim65Controller->Start();

    return app.exec();
}
