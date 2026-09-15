#pragma once
#include "playbackworker.h"
#include "playlistmodel.h"
#include <QThread>

enum class PlayMode { Sequential, LoopAll, LoopOne, Shuffle };

class PlayerController : public QObject
{
    Q_OBJECT
public:
    explicit PlayerController(PlaylistModel *playlist, QObject *parent = nullptr);
    ~PlayerController() override;
    void play(int index);
    void toggle(int selectedIndex = -1);
    void stop();
    void next();
    void previous();
    void seek(qint64 ms);
    void setVolume(int percent);
    void setPlayMode(PlayMode mode);
    PlayMode playMode() const { return m_mode; }
    PlaybackState state() const { return m_state; }
    int currentIndex() const { return m_playlist->indexOf(m_currentFile); }
    QString currentFile() const { return m_currentFile; }
    qint64 duration() const { return m_duration; }
    static QString modeName(PlayMode mode);
    static int nextIndex(int current, int count, PlayMode mode, bool automatic);
signals:
    void stateChanged(PlaybackState state);
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void currentTrackChanged(int index);
    void songChanged(const QString &title, const QString &artist, int rate, int channels, int bitrate);
    void errorOccurred(const QString &message);
    void modeChanged(PlayMode mode);
    void openRequested(const QString &path, quint64 generation);
    void stopRequested(quint64 generation);
    void pauseRequested(quint64 generation);
    void resumeRequested(quint64 generation);
    void seekRequested(qint64 ms, quint64 generation);
    void volumeRequested(int percent);
private:
    void setState(PlaybackState state);
    PlaylistModel *m_playlist;
    QThread m_thread;
    PlaybackWorker *m_worker;
    QString m_currentFile;
    PlaybackState m_state = PlaybackState::Stopped;
    PlayMode m_mode = PlayMode::Sequential;
    quint64 m_generation = 0;
    qint64 m_duration = 0;
};
