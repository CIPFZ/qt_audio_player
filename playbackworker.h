#pragma once
#include "audiodecoder.h"
#include "audioplayer.h"
#include <QObject>
#include <QTimer>

enum class PlaybackState { Stopped, Loading, Playing, Paused };
Q_DECLARE_METATYPE(PlaybackState)

class PlaybackWorker : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackWorker(QObject *parent = nullptr);
public slots:
    void open(const QString &path, quint64 generation);
    void stop(quint64 generation);
    void pause(quint64 generation);
    void resume(quint64 generation);
    void seek(qint64 ms, quint64 generation);
    void setVolume(int value) { m_player.setVolume(value); }
    void shutdown();
signals:
    void stateChanged(PlaybackState state, quint64 generation);
    void positionChanged(qint64 ms, quint64 generation);
    void metadataReady(const QString &title, const QString &artist, qint64 duration,
                       int sampleRate, int channels, int bitrate, quint64 generation);
    void finished(quint64 generation);
    void errorOccurred(const QString &message, quint64 generation);
private:
    void tick();
    bool fillBuffer();
    void fail(const QString &message);
    void setState(PlaybackState state);
    AudioDecoder m_decoder;
    AudioPlayer m_player;
    QTimer *m_timer;
    std::vector<float> m_pending;
    size_t m_pendingOffset = 0;
    PlaybackState m_state = PlaybackState::Stopped;
    quint64 m_generation = 0;
    qint64 m_duration = 0;
    qint64 m_positionBase = 0;
    qint64 m_lastPosition = -1;
    bool m_eof = false;
};
