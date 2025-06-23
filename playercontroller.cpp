#include "playercontroller.h"
#include <QDebug>

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
{
    // 进度更新定时器，建议 200ms 一次
    m_timer.setInterval(200);
    connect(&m_timer, &QTimer::timeout, this, &PlayerController::onPlaybackTimer);
    connect(&m_player, &AudioPlayer::playFinished, this, &PlayerController::onPlayFinished);
}

void PlayerController::setPlaylist(const QStringList &fileList) {
    m_playlist = fileList;
    m_currentIndex = -1;
}

void PlayerController::play(int index) {
    if (index < 0 || index >= m_playlist.size()) {
        qWarning() << "播放索引越界:" << index;
        return;
    }
    m_currentIndex = index;

    stop(); // 先停止当前播放

    QString filePath = m_playlist[m_currentIndex];
    if (!m_decoder.openFile(filePath)) {
        qWarning() << "无法打开文件:" << filePath;
        return;
    }

    m_duration = m_decoder.getDurationMs();
    emit durationChanged(m_duration);

    if (!m_player.start(m_decoder.getSampleRate(), m_decoder.getChannels())) {
        qWarning() << "AudioPlayer 启动失败";
        return;
    }

    m_position = 0;
    m_isPlaying = true;
    emit playbackStateChanged(true);

    m_timer.start();

    startDecodingAndPlaying();

    emit currentSongChanged(
        QFileInfo(filePath).fileName(),
        m_decoder.getSampleRate(),
        m_decoder.getChannels(),
        m_decoder.getBitrate() / 1000
    );
}

void PlayerController::stop() {
    m_timer.stop();
    m_player.stop();
    m_decoder.cleanup();
    m_position = 0;
    m_isPlaying = false;
    emit playbackStateChanged(false);
}

bool PlayerController::isPlaying()
{
    return m_isPlaying;
}

void PlayerController::setVolume(int percent)
{
    m_player.setVolume(percent);
}

void PlayerController::pause() {
    if (m_isPlaying) {
        m_player.pause();
        m_timer.stop();
        m_isPlaying = false;
        emit playbackStateChanged(false);
    }
}

void PlayerController::resume() {
    if (!m_isPlaying) {
        m_player.resume();
        m_timer.start();
        m_isPlaying = true;
        emit playbackStateChanged(true);
    }
}

void PlayerController::seek(qint64 ms) {
    if (!m_decoder.seek(ms)) {
        qWarning() << "seek 失败";
        return;
    }
    m_position = ms;
    // 清空播放缓冲，重新解码
    m_player.clearPcmData();
    startDecodingAndPlaying();
    emit positionChanged(m_position);
}

QString PlayerController::currentFile() const {
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size())
        return m_playlist[m_currentIndex];
    return QString();
}

void PlayerController::onPlaybackTimer() {
    m_position += 200; // 估算播放进度，单位ms

    if (m_position >= m_duration) {
        // 歌曲播放结束
        stop();
        emit positionChanged(m_duration);
        return;
    }

    emit positionChanged(m_position);
}

void PlayerController::startDecodingAndPlaying() {
    constexpr size_t bufferSize = 4096 * 8;
    std::vector<uint8_t> pcmBuffer(bufferSize);

    int outSampleRate = 0;
    int outChannels = 0;

    while (m_isPlaying) {
        int decodedBytes = m_decoder.decodeFrame(pcmBuffer.data(), bufferSize, outSampleRate, outChannels);
        if (decodedBytes <= 0) {
            break;
        }
        m_player.addPcmData(pcmBuffer.data(), decodedBytes);
    }
}

void PlayerController::onPlayFinished() {
    qDebug() << "play finish ---->";
    emit playCompleted();
}

