#include "audiodecoder.h"
#include "audioplayer.h"
#include "libraryimporter.h"
#include "pcmbuffer.h"
#include "playercontroller.h"
#include "playliststore.h"
#include "fixtures.h"
#include "fakeportaudio.h"
#include <QAbstractItemModelTester>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <thread>

namespace {
int runCommand(const QString &program, const QStringList &arguments)
{
    QProcess process;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0) && defined(Q_OS_UNIX)
    // A child modifier makes Qt use fork(), which also works on hosts with vfork hooks.
    process.setChildProcessModifier([] {});
#endif
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(program,arguments);
    if (!process.waitForStarted(5000) || !process.waitForFinished(15000)) {
        process.kill(); process.waitForFinished(); return -1;
    }
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}
}

class CoreTest : public QObject
{
    Q_OBJECT
private slots:
    void ringWrapGainAndUnderrun();
    void concurrentRing();
    void modelAndStore();
    void recursiveImport();
    void decoderResampleAndSeek();
    void decoderCompressed_data();
    void decoderCompressed();
    void playbackModes();
    void streamUnderrunIsNotEnd();
    void pauseSeekResumeAndCompletion();
    void editsAndRapidSwitching();
    void missingOutputDevice();
};
void CoreTest::ringWrapGainAndUnderrun()
{
    PcmBuffer buffer(8);
    const float samples[]{.2f,.4f,.6f,.8f,1.0f,-1.0f};
    float output[8]{};
    QCOMPARE(buffer.write(samples,6),6u);
    QCOMPARE(buffer.read(output,4,.5f),4u);
    QVERIFY(std::abs(output[0]-.1f)<0.0001f);
    QCOMPARE(buffer.write(samples,6),6u);
    QCOMPARE(buffer.available(),8u);
    QCOMPARE(buffer.write(samples,2),0u);
    QCOMPARE(buffer.read(output,8,1.0f),8u);
    QCOMPARE(output[0],1.0f); QCOMPARE(output[1],-1.0f); QCOMPARE(output[7],-1.0f);
    QCOMPARE(buffer.read(output,8,1.0f),0u);
    for(float sample:output) QCOMPARE(sample,0.0f);
}
void CoreTest::concurrentRing()
{
    PcmBuffer buffer(1024);
    constexpr int count=200000;
    std::atomic_bool valid{true};
    std::thread producer([&] {
        for(int i=0;i<count;) { float value=float(i%100)/100; if(buffer.write(&value,1)) ++i; else std::this_thread::yield(); }
    });
    std::thread consumer([&] {
        for(int i=0;i<count;) { float value; if(buffer.read(&value,1,1)) { if(value!=float(i%100)/100) valid=false; ++i; } else std::this_thread::yield(); }
    });
    producer.join(); consumer.join();
    QVERIFY(valid); QCOMPARE(buffer.available(),0u);
}
void CoreTest::modelAndStore()
{
    QTemporaryDir directory;
    const auto a=directory.filePath("alpha.wav"), b=directory.filePath("beta.WAV");
    QVERIFY(writeWave(a)); QVERIFY(writeWave(b));
    PlaylistModel model;
    QAbstractItemModelTester modelTester(&model,QAbstractItemModelTester::FailureReportingMode::QtTest);
    QCOMPARE(model.addFiles({a,b,a,directory.filePath("missing.wav")}),2);
    QCOMPARE(model.pathAt(-1),QString());
    model.setCurrentPath(b); model.selectPath(a);
    QCOMPARE(model.data(model.index(1,0),PlaylistModel::CurrentRole).toBool(),true);
    QVERIFY(model.removeRow(0)); QCOMPARE(model.pathAt(0),b); QCOMPARE(model.selectedPaths().size(),0);
    QVERIFY(!model.removeRows(0,2));
    QString error; const auto file=directory.filePath("nested/playlist.json");
    QVERIFY(PlaylistStore::save(file,model.paths(),error));
    QStringList loaded; QVERIFY(PlaylistStore::load(file,loaded,error)); QCOMPARE(loaded,model.paths());
    QFile broken(file); QVERIFY(broken.open(QIODevice::WriteOnly)); broken.write("{bad json"); broken.close();
    QVERIFY(!PlaylistStore::load(file,loaded,error)); QVERIFY(!error.isEmpty());
    QVERIFY(broken.open(QIODevice::ReadOnly)); QCOMPARE(broken.readAll(),QByteArray("{bad json"));
}
void CoreTest::recursiveImport()
{
    QTemporaryDir directory;
    QDir().mkpath(directory.filePath("album/sub"));
    QVERIFY(writeWave(directory.filePath("album/sub/test.WAV")));
    QFile unsupported(directory.filePath("notes.txt")); QVERIFY(unsupported.open(QIODevice::WriteOnly)); unsupported.write("text"); unsupported.close();
    auto cancelled=std::make_shared<std::atomic_bool>(false);
    const auto files=scanAudioPaths({directory.path(),directory.filePath("album/sub/test.WAV")},cancelled);
    QCOMPARE(files.size(),1);
    cancelled->store(true); QVERIFY(scanAudioPaths({directory.path()},cancelled).isEmpty());
}
void CoreTest::decoderResampleAndSeek()
{
    QTemporaryDir directory;
    const auto path=directory.filePath("声音.wav");
    QVERIFY(writeWave(path,1000));
    AudioDecoder decoder;
    QVERIFY2(decoder.openFile(path),qPrintable(decoder.errorString()));
    QCOMPARE(decoder.sampleRate(),22050); QCOMPARE(decoder.channels(),1); QCOMPARE(decoder.duration(),1000);
    QCOMPARE(decoder.bitrateKbps(),352);
    std::vector<float> pcm; qint64 total=0;
    for (;;) {
        const auto result=decoder.decode(pcm);
        QVERIFY2(result!=AudioDecoder::Result::Error,qPrintable(decoder.errorString()));
        if(result==AudioDecoder::Result::End) break;
        total+=pcm.size()/2;
        for(float sample:pcm) QVERIFY(std::isfinite(sample));
    }
    QVERIFY(std::abs(total-48000)<=1);
    QVERIFY(decoder.seek(400)); total=0;
    while(decoder.decode(pcm)==AudioDecoder::Result::Data) total+=pcm.size()/2;
    QVERIFY2(std::abs(total-28800)<=2,qPrintable(QString::number(total)));
    QVERIFY(decoder.seek(1000)); QCOMPARE(decoder.decode(pcm),AudioDecoder::Result::End);
    QVERIFY(!decoder.openFile(directory.filePath("missing.wav")));
    QVERIFY(decoder.openFile(path));
}
void CoreTest::decoderCompressed_data()
{
    QTest::addColumn<QString>("extension");
    QTest::newRow("mp3") << "mp3";
    QTest::newRow("aac") << "m4a";
    QTest::newRow("flac") << "flac";
    QTest::newRow("opus") << "opus";
}
void CoreTest::decoderCompressed()
{
    QFETCH(QString,extension);
    const auto ffmpeg=QStandardPaths::findExecutable("ffmpeg");
    if(ffmpeg.isEmpty()) QSKIP("ffmpeg CLI is needed for codec regression fixtures");
    QTemporaryDir directory;
    const auto source=directory.filePath("source.wav"), input=directory.filePath("source."+extension), reference=directory.filePath("reference.pcm");
    QVERIFY(writeWave(source,120,44100,2));
    QCOMPARE(runCommand(ffmpeg,{"-v","error","-y","-i",source,"-metadata","title=Decoder fixture",input}),0);
    QCOMPARE(runCommand(ffmpeg,{"-v","error","-y","-i",input,"-ar","48000","-ac","2","-f","f32le",reference}),0);
    AudioDecoder decoder; QVERIFY2(decoder.openFile(input),qPrintable(decoder.errorString()));
    if(extension!="opus") QCOMPARE(decoder.title(),QString("Decoder fixture"));
    qint64 samples=0; std::vector<float> pcm;
    for(;;) { const auto result=decoder.decode(pcm); QVERIFY(result!=AudioDecoder::Result::Error); if(result==AudioDecoder::Result::End) break; samples+=pcm.size(); }
    QCOMPARE(samples*qint64(sizeof(float)),QFileInfo(reference).size());
}
void CoreTest::playbackModes()
{
    QCOMPARE(PlayerController::nextIndex(0,0,PlayMode::Sequential,true),-1);
    QCOMPARE(PlayerController::nextIndex(1,2,PlayMode::Sequential,true),-1);
    QCOMPARE(PlayerController::nextIndex(1,2,PlayMode::LoopAll,true),0);
    QCOMPARE(PlayerController::nextIndex(1,2,PlayMode::LoopOne,true),1);
    QCOMPARE(PlayerController::nextIndex(1,2,PlayMode::LoopOne,false),0);
    for(int i=0;i<100;++i) QVERIFY(PlayerController::nextIndex(1,4,PlayMode::Shuffle,true)!=1);
    QCOMPARE(PlayerController::nextIndex(0,1,PlayMode::Shuffle,true),0);
}
void CoreTest::streamUnderrunIsNotEnd()
{
    AudioPlayer output; QVERIFY(output.open()); QVERIFY(output.start());
    QTest::qWait(70); QCOMPARE(output.streamStatus(),1); QCOMPARE(output.positionMs(),0);
    float pcm[2048]{}; QCOMPARE(output.write(pcm,2048),2048u);
    QTRY_VERIFY(output.positionMs()>0);
    QVERIFY(!output.finished());
    output.setEndOfInput(); QTRY_VERIFY(output.finished());
}
void CoreTest::pauseSeekResumeAndCompletion()
{
    QTemporaryDir directory; const auto path=directory.filePath("song.wav"); QVERIFY(writeWave(path,1500));
    PlaylistModel model; model.addFiles({path}); PlayerController player(&model);
    QSignalSpy positions(&player,&PlayerController::positionChanged);
    QSignalSpy errors(&player,&PlayerController::errorOccurred);
    player.play(0); QTRY_COMPARE(player.state(),PlaybackState::Playing);
    QTRY_VERIFY(positions.last().at(0).toLongLong()>120);
    player.toggle(); QTRY_COMPARE(player.state(),PlaybackState::Paused);
    const auto paused=positions.last().at(0).toLongLong(); QTest::qWait(120);
    QCOMPARE(positions.last().at(0).toLongLong(),paused);
    player.seek(800); QTRY_COMPARE(positions.last().at(0).toLongLong(),800);
    QCOMPARE(player.state(),PlaybackState::Paused);
    player.toggle(); QTRY_COMPARE(player.state(),PlaybackState::Playing);
    QTRY_VERIFY(positions.last().at(0).toLongLong()>800);
    QTRY_COMPARE(player.state(),PlaybackState::Stopped);
    QCOMPARE(positions.last().at(0).toLongLong(),1500); QVERIFY(errors.isEmpty());
}
void CoreTest::editsAndRapidSwitching()
{
    QTemporaryDir directory; const auto a=directory.filePath("a.wav"), b=directory.filePath("b.wav");
    QVERIFY(writeWave(a,1600)); QVERIFY(writeWave(b,1600));
    PlaylistModel model; model.addFiles({a,b}); PlayerController player(&model);
    QSignalSpy positions(&player,&PlayerController::positionChanged);
    player.play(1); QTRY_COMPARE(player.state(),PlaybackState::Playing);
    model.removeRow(0); QCOMPARE(player.currentIndex(),0); QCOMPARE(player.state(),PlaybackState::Playing);
    model.addFiles({a}); QCOMPARE(player.currentIndex(),0); QCOMPARE(player.currentFile(),b);
    player.play(1); player.play(0); player.stop();
    QTest::qWait(120); QCOMPARE(player.state(),PlaybackState::Stopped); QCOMPARE(positions.last().at(0).toLongLong(),0);
    player.play(0); QTRY_COMPARE(player.state(),PlaybackState::Playing);
    model.removeRow(0); QCOMPARE(player.state(),PlaybackState::Stopped); QCOMPARE(player.currentIndex(),-1);
    player.setPlayMode(PlayMode::LoopAll); model.removeRow(0); player.next(); QCOMPARE(player.currentIndex(),-1);
}
void CoreTest::missingOutputDevice()
{
    FakeAudio::setAvailable(false);
    AudioPlayer output; QVERIFY(!output.open()); QVERIFY(!output.errorString().isEmpty());
    FakeAudio::setAvailable(true); QVERIFY(output.open());
}
QTEST_GUILESS_MAIN(CoreTest)
#include "test_core.moc"
