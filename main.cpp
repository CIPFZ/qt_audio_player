#include "librarycontroller.h"
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication application(argc, argv);
    application.setApplicationName("AudioPlayer");
    application.setOrganizationName("BitZion");
    application.setApplicationVersion("2.0.0");
    application.setWindowIcon(QIcon(":/icons/logo.png"));
    QQuickStyle::setStyle("Basic");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickStyle::setStyle("Default");
#endif
    LibraryController library;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("libraryController", &library);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    const auto paths = application.arguments().mid(1);
    if (!paths.isEmpty()) QTimer::singleShot(0,&library,[&library,paths] { library.importPaths(paths); });
    return application.exec();
}
