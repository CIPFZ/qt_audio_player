#include "playbackworker.h"
#include <QFileInfo>

PlaybackWorker::PlaybackWorker(QObject *parent) : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setInterval(10);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &PlaybackWorker::tick);
}
void PlaybackWorker::setState(PlaybackState state)
{
    m_state = state;
    emit stateChanged(state, m_generation);
}
void PlaybackWorker::shutdown()
{
    m_timer->stop();
    m_player.close();
    m_decoder.close();
    m_pending.clear();
    m_pendingOffset = 0;
    m_eof = false;
    m_state = PlaybackState::Stopped;
}
void PlaybackWorker::stop(quint64 generation)
{
    shutdown();
    m_generation = generation;
    setState(PlaybackState::Stopped);
    emit positionChanged(0, m_generation);
}
void PlaybackWorker::fail(const QString &message)
{
    shutdown();
    setState(PlaybackState::Stopped);
    emit errorOccurred(message, m_generation);
}
void PlaybackWorker::open(const QString &path, quint64 generation)
{
    shutdown();
    m_generation = generation;
    setState(PlaybackState::Loading);
    if (!m_decoder.openFile(path)) { fail(m_decoder.errorString()); return; }
    m_duration = m_decoder.duration();
    m_positionBase = 0;
    m_lastPosition = -1;
    emit metadataReady(m_decoder.title().isEmpty() ? QFileInfo(path).completeBaseName() : m_decoder.title(),
                       m_decoder.artist(), m_duration, m_decoder.sampleRate(), m_decoder.channels(),
                       m_decoder.bitrateKbps(), m_generation);
    if (!m_player.open()) { fail(m_player.errorString()); return; }
    // Prefill before starting the callback. The decoder only stays ~300 ms ahead.
    if (!fillBuffer()) return;
    if (!m_player.start()) { fail(m_player.errorString()); return; }
    setState(PlaybackState::Playing);
    emit positionChanged(0, m_generation);
    m_timer->start();
}
bool PlaybackWorker::fillBuffer()
{
    constexpr uint32_t targetSamples = AudioDecoder::OutputRate * 2 * 300 / 1000;
    // Bound work per tick so queued pause/seek/stop commands remain responsive.
    for (int block = 0; block < 24 && m_player.bufferedSamples() < targetSamples; ++block) {
        if (m_pendingOffset < m_pending.size()) {
            const auto written = m_player.write(m_pending.data() + m_pendingOffset,
                                                 uint32_t(m_pending.size() - m_pendingOffset));
            m_pendingOffset += written;
            if (m_pendingOffset < m_pending.size()) break;
        }
        if (m_eof) break;
        const auto result = m_decoder.decode(m_pending);
        m_pendingOffset = 0;
        if (result == AudioDecoder::Result::Error) { fail(m_decoder.errorString()); return false; }
        if (result == AudioDecoder::Result::End) {
            m_eof = true;
            m_player.setEndOfInput();
            break;
        }
    }
    return true;
}
void PlaybackWorker::tick()
{
    if (m_state != PlaybackState::Playing || !fillBuffer()) return;
    const qint64 position = m_positionBase + m_player.positionMs();
    if (position - m_lastPosition >= 80) {
        m_lastPosition = position;
        emit positionChanged(m_duration > 0 ? std::min(position, m_duration) : position, m_generation);
    }
    if (m_player.finished()) {
        const auto endPosition = m_duration > 0 ? m_duration : position;
        shutdown();
        emit positionChanged(endPosition, m_generation);
        setState(PlaybackState::Stopped);
        emit finished(m_generation);
    } else if (m_player.streamStatus() != 1) {
        fail(QStringLiteral("音频输出已中断，请检查默认输出设备后重新播放。"));
    }
}
void PlaybackWorker::pause(quint64 generation)
{
    if (generation != m_generation || m_state != PlaybackState::Playing) return;
    if (!m_player.pause()) { fail(m_player.errorString()); return; }
    m_timer->stop();
    emit positionChanged(m_positionBase + m_player.positionMs(), m_generation);
    setState(PlaybackState::Paused);
}
void PlaybackWorker::resume(quint64 generation)
{
    if (generation != m_generation || m_state != PlaybackState::Paused) return;
    if (!m_player.start()) { fail(m_player.errorString()); return; }
    setState(PlaybackState::Playing);
    m_timer->start();
}
void PlaybackWorker::seek(qint64 ms, quint64 generation)
{
    if (generation != m_generation || (m_state != PlaybackState::Playing && m_state != PlaybackState::Paused)) return;
    const bool wasPlaying = m_state == PlaybackState::Playing;
    if (!m_player.pause()) { fail(m_player.errorString()); return; }
    m_timer->stop();
    ms = std::max<qint64>(0, m_duration > 0 ? std::min(ms, m_duration) : ms);
    if (!m_decoder.seek(ms)) { fail(m_decoder.errorString()); return; }
    // Pa_StopStream has joined the callback; resetting the SPSC ring is now safe.
    m_player.reset();
    m_pending.clear();
    m_pendingOffset = 0;
    m_eof = false;
    m_positionBase = ms;
    m_lastPosition = ms;
    if (!fillBuffer()) return;
    emit positionChanged(ms, m_generation);
    if (wasPlaying) {
        if (!m_player.start()) { fail(m_player.errorString()); return; }
        m_timer->start();
    }
}
