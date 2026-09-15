#include "librarycontroller.h"
#include "fixtures.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class QmlTest : public QObject
{
    Q_OBJECT
private slots:
    void libraryWorkflow();
    void settingsSurviveRestart();
};
void QmlTest::libraryWorkflow()
{
    QTemporaryDir music;
    const QStringList titles{QStringLiteral("01 - 午后的风"),QStringLiteral("02 - 山间来信"),QStringLiteral("03 - Blue Hour"),
                             QStringLiteral("04 - 慢慢走"),QStringLiteral("05 - A Quiet Place"),QStringLiteral("06 - 月光与海")};
    QStringList paths;
    for(const auto &title:titles) { const auto path=music.filePath(title+".wav"); QVERIFY(writeWave(path,4000,44100,2)); paths.append(path); }
    LibraryController library;
    QQmlApplicationEngine engine;
    QStringList warnings;
    connect(&engine,&QQmlEngine::warnings,this,[&warnings](const QList<QQmlError> &errors) {
        for(const auto &error:errors) warnings.append(error.toString());
    });
    engine.rootContext()->setContextProperty("libraryController",&library);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    QVERIFY2(!engine.rootObjects().isEmpty(),"QML root failed to load");
    auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto item=[window](const char *name) { return window->findChild<QQuickItem *>(QString::fromLatin1(name)); };
    auto click=[window](QQuickItem *control) {
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,control->mapToScene(QPointF(control->width()/2,control->height()/2)).toPoint());
    };
    auto screenshot=[window](const QString &name) {
        const auto directory=qEnvironmentVariable("PLAYER_SCREENSHOT_DIR");
        if(directory.isEmpty()) return;
        QDir().mkpath(directory);
        QTest::qWait(180);
        auto image=window->grabWindow();
        if(image.isNull()) image=window->screen()->grabWindow(window->winId()).toImage();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(directory+"/"+name+".png"));
    };
    QCOMPARE(library.trackCount(),0);
    QVERIFY(item("playButton")); QVERIFY(!item("playButton")->isEnabled());
    screenshot("qml-empty");
    library.importPaths(paths);
    QTRY_COMPARE(library.trackCount(),6);
    QTRY_COMPARE(item("trackList")->property("count").toInt(),6);
    QVERIFY(item("playButton")->isEnabled());
    library.selectRow(0); library.playRow(0);
    QTRY_VERIFY(library.playing());
    QTRY_VERIFY(library.position()>150);
    QCOMPARE(library.title(),titles.first());
    QVERIFY(library.audioInfo().contains("44100"));
    click(item("playButton")); QTRY_VERIFY(!library.playing());
    QCOMPARE(library.stateText(),QStringLiteral("已暂停"));
    const auto paused=library.position(); QTest::qWait(100); QCOMPARE(library.position(),paused);
    auto *slider=item("progressSlider");
    const auto seekPoint=slider->mapToScene(QPointF(slider->width()*0.5,slider->height()/2)).toPoint();
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,seekPoint);
    QTRY_VERIFY(library.position()>=1800 && library.position()<=2200);
    QVERIFY(!library.playing());
    click(item("playButton")); QTRY_VERIFY(library.playing());
    QTRY_VERIFY(library.position()>2200);
    click(item("playButton")); QTRY_VERIFY(!library.playing());
    click(item("modeButton")); QCOMPARE(library.playMode(),1);
    click(item("muteButton")); QCOMPARE(library.volume(),0);
    click(item("muteButton")); QCOMPARE(library.volume(),50);
    screenshot("qml-library");
    library.setSearchText("Blue"); QTRY_COMPARE(library.visibleCount(),1);
    QCOMPARE(item("searchField")->property("text").toString(),QString("Blue"));
    library.playRow(0); QTRY_VERIFY(library.playing()); QCOMPARE(library.title(),titles.at(2));
    item("searchField")->forceActiveFocus();
    QTest::keyClick(window,Qt::Key_Space);
    QVERIFY(library.playing());
    library.setSearchText("no matching song"); QCOMPARE(library.visibleCount(),0);
    library.setSearchText("");
    library.selectRow(0); library.selectRow(1,true); QCOMPARE(library.selectionCount(),2);
    library.removeSelected(); QCOMPARE(library.trackCount(),4); QVERIFY(library.playing());
    QVERIFY(QFileInfo::exists(paths.first()));
    library.revealCurrent(); QCOMPARE(library.selectionCount(),1);
    library.removeSelected(); QTRY_VERIFY(!library.playing());
    window->resize(880,640); QTest::qWait(100);
    screenshot("qml-compact");
    QVERIFY(item("playButton")->mapToScene(QPointF()).x()>=0);
    QVERIFY(item("volumeSlider")->mapToScene(QPointF(item("volumeSlider")->width(),0)).x()<=window->width());
    library.setVolume(37); library.saveWindowSize(1024,720);
    for(const auto &warning:warnings) qWarning().noquote() << warning;
    QCOMPARE(warnings.size(),0);
}
void QmlTest::settingsSurviveRestart()
{
    LibraryController library;
    QCOMPARE(library.volume(),37);
    QCOMPARE(library.playMode(),1);
    QCOMPARE(library.savedWidth(),1024);
    QCOMPARE(library.savedHeight(),720);
    // All files from the prior session were temporary and should be pruned safely.
    QCOMPARE(library.trackCount(),0);
}
int main(int argc,char **argv)
{
    QTemporaryDir state;
    qputenv("XDG_CONFIG_HOME",(state.path()+"/config").toUtf8());
    qputenv("XDG_DATA_HOME",(state.path()+"/data").toUtf8());
    QApplication app(argc,argv);
    app.setOrganizationName("AudioPlayerTests"); app.setApplicationName("QmlIntegration");
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QQuickStyle::setStyle("Default");
#else
    QQuickStyle::setStyle("Basic");
#endif
    QmlTest test;
    return QTest::qExec(&test,argc,argv);
}
#include "test_qml.moc"
