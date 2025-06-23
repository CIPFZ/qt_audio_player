#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QFileInfo>
#include "audioplayer.h"
#include "audiodecoder.h" // 你自己的解码器类

enum class PlayMode {
    Sequential,     // 顺序播放
    LoopOne,        // 单曲循环
    Shuffle         // 随机播放
};


class PlayerController : public QObject
{
    Q_OBJECT
public:
    explicit PlayerController(QObject *parent = nullptr);

    void setPlaylist(const QStringList &fileList);
    void play(int index = 0);
    void pause();
    void resume();
    void stop();
    bool isPlaying();
    void setVolume(int percent);

    int currentIndex() const { return m_currentIndex; }
    QString currentFile() const;

signals:
    void positionChanged(qint64 ms); // 当前播放进度
    void durationChanged(qint64 ms); // 当前文件总时长
    void playbackStateChanged(bool isPlaying);
    void playCompleted();
    void currentSongChanged(const QString &title, int sampleRate, int channels, int bitrateKbps);

public slots:
    void seek(qint64 ms);

private slots:
    void onPlaybackTimer();

private:
    AudioDecoder m_decoder; // 你现有的解码器
    AudioPlayer m_player;   // 你现有的音频播放类

    QStringList m_playlist;
    int m_currentIndex = -1;

    QTimer m_timer; // 定时器，用于更新进度条

    qint64 m_duration = 0; // 总时长(ms)
    qint64 m_position = 0; // 当前播放位置(ms)

    bool m_isPlaying = false;

    void startDecodingAndPlaying();
    void onPlayFinished();
};

#endif // PLAYERCONTROLLER_H
