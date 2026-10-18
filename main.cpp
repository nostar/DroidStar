#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QQmlContext>
#include "droidstar.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    // Keep Qt event loops active when Android moves the Activity to the
    // background. This complements android.app.background_running in the
    // manifest; the foreground service keeps the process eligible to run.
    qputenv("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED", "0");
#endif

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Fusion");
    app.setWindowIcon(QIcon(":/images/droidstar.png"));
    qmlRegisterType<DroidStar>("org.dudetronics.droidstar", 1, 0, "DroidStar");
    QQmlApplicationEngine engine;
#ifdef USE_FLITE
    engine.rootContext()->setContextProperty("USE_FLITE", QVariant(true));
#else
    engine.rootContext()->setContextProperty("USE_FLITE", QVariant(false));
#endif

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("DroidStarApp", "Main");
    return app.exec();

}
