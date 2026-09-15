#include "playercontroller.h"
#include <QRandomGenerator>
#include <QFileInfo>

PlayerController::PlayerController(PlaylistModel *playlist, QObject *parent)
    : QObject(parent), m_playlist(playlist), m_worker(new PlaybackWorker)
{
    qRegisterMetaType<PlaybackState>();
    m_thread.setObjectName(QStringLiteral("Audio playback"));
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &PlayerController::openRequested, m_worker, &PlaybackWorker::open);
    connect(this, &PlayerController::stopRequested, m_worker, &PlaybackWorker::stop);
    connect(this, &PlayerController::pauseRequested, m_worker, &PlaybackWorker::pause);
    connect(this, &PlayerController::resumeRequested, m_worker, &PlaybackWorker::resume);
    connect(this, &PlayerController::seekRequested, m_worker, &PlaybackWorker::seek);
    connect(this, &PlayerController::volumeRequested, m_worker, &PlaybackWorker::setVolume);
    connect(m_worker, &PlaybackWorker::stateChanged, this, [this](PlaybackState state, quint64 id) {
        if (id == m_generation) setState(state);
    });
    connect(m_worker, &PlaybackWorker::positionChanged, this, [this](qint64 ms, quint64 id) {
        if (id == m_generation) emit positionChanged(ms);
    });
    connect(m_worker, &PlaybackWorker::metadataReady, this,
            [this](const QString &title, const QString &artist, qint64 duration, int rate, int channels, int bitrate, quint64 id) {
        if (id != m_generation) return;
        m_duration = duration;
        m_playlist->updateMetadata(m_currentFile, title, artist);
        emit durationChanged(duration);
        emit songChanged(title, artist, rate, channels, bitrate);
    });
    connect(m_worker, &PlaybackWorker::errorOccurred, this, [this](const QString &error, quint64 id) {
        if (id != m_generation) return;
        setState(PlaybackState::Stopped);
        emit errorOccurred(error);
    });
    connect(m_worker, &PlaybackWorker::finished, this, [this](quint64 id) {
        if (id != m_generation) return;
        const int next = nextIndex(currentIndex(), m_playlist->rowCount(), m_mode, true);
        if (next >= 0) play(next);
    });
    connect(m_playlist, &PlaylistModel::tracksChanged, this, [this] {
        if (!m_currentFile.isEmpty() && currentIndex() < 0) {
            stop();
            m_currentFile.clear();
            m_playlist->setCurrentPath({});
            m_duration = 0;
            emit durationChanged(0);
            emit songChanged(tr("还没有选择音乐"), tr("从你的音乐库开始"), 0, 0, 0);
        }
        emit currentTrackChanged(currentIndex());
    });
    m_thread.start();
}
PlayerController::~PlayerController()
{
    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
}
void PlayerController::setState(PlaybackState state)
{
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}
void PlayerController::play(int index)
{
    const auto path = m_playlist->pathAt(index);
    if (path.isEmpty()) return;
    ++m_generation;
    m_currentFile = path;
    m_duration = 0;
    m_playlist->setCurrentPath(path);
    emit currentTrackChanged(index);
    emit durationChanged(0);
    emit positionChanged(0);
    emit songChanged(QFileInfo(path).completeBaseName(), {}, 0, 0, 0);
    setState(PlaybackState::Loading);
    emit openRequested(path, m_generation);
}
void PlayerController::toggle(int selectedIndex)
{
    if (m_state == PlaybackState::Playing) emit pauseRequested(m_generation);
    else if (m_state == PlaybackState::Paused) emit resumeRequested(m_generation);
    else if (m_state == PlaybackState::Stopped)
        play(selectedIndex >= 0 ? selectedIndex : (currentIndex() >= 0 ? currentIndex() : 0));
}
void PlayerController::stop()
{
    emit stopRequested(++m_generation);
    setState(PlaybackState::Stopped);
    emit positionChanged(0);
}
int PlayerController::nextIndex(int current, int count, PlayMode mode, bool automatic)
{
    if (count <= 0) return -1;
    if (current < 0 || current >= count) return 0;
    if (mode == PlayMode::LoopOne && automatic) return current;
    if (mode == PlayMode::Shuffle && count > 1) {
        const int choice = QRandomGenerator::global()->bounded(count - 1);
        return choice >= current ? choice + 1 : choice;
    }
    if (automatic && mode == PlayMode::Sequential && current == count - 1) return -1;
    return (current + 1) % count;
}
void PlayerController::next() { play(nextIndex(currentIndex(), m_playlist->rowCount(), m_mode, false)); }
void PlayerController::previous()
{
    const int count = m_playlist->rowCount();
    if (count == 0) return;
    if (m_mode == PlayMode::Shuffle) next();
    else play(currentIndex() <= 0 ? count - 1 : currentIndex() - 1);
}
void PlayerController::seek(qint64 ms)
{
    if (m_state == PlaybackState::Playing || m_state == PlaybackState::Paused) emit seekRequested(ms, m_generation);
}
void PlayerController::setVolume(int percent) { emit volumeRequested(std::clamp(percent, 0, 100)); }
void PlayerController::setPlayMode(PlayMode mode)
{
    m_mode = mode;
    emit modeChanged(mode);
}
QString PlayerController::modeName(PlayMode mode)
{
    switch (mode) {
    case PlayMode::Sequential: return tr("顺序播放");
    case PlayMode::LoopAll: return tr("列表循环");
    case PlayMode::LoopOne: return tr("单曲循环");
    case PlayMode::Shuffle: return tr("随机播放");
    }
    return {};
}
