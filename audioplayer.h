#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <cstdint>
#include <cstring>
#include <QtDebug>
#include <portaudio.h>
#include <QObject>
#include <QDebug>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque> // 用于缓冲音频数据


// 定义一个缓冲区块大小，实际应用中可以根据需求调整
const int AUDIO_BUFFER_CHUNK_SIZE = 4096; // 4KB

template <typename T>
T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}


class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();

    // PortAudio 初始化和设备管理
    void listPortAudioDevices();
    void setOutputDevice(int deviceIndex);
    int getOutputDeviceIndex() const { return outputDeviceIndex; }

    // 开始播放
    bool start(int sampleRate, int channels);
    // 停止播放
    void stop();
    // 暂停播放
    void pause();
    // 恢复播放
    bool resume();

    // 检查是否正在播放（流已打开且处于活跃状态）
    bool isPlaying() const;
    // 检查是否已暂停（流已打开但未活跃）
    bool isPaused() const { return stream != nullptr && !m_isStreamActive; }


    // 将PCM数据块添加到播放缓冲区
    void addPcmData(const uint8_t *data, size_t size);
    // 清空播放缓冲区
    void clearPcmData();
    // 获取缓冲区中剩余的字节数
    size_t getBufferedBytes() const;
    // 获取缓冲区阈值（用于判断何时需要更多数据）
    size_t getBufferThresholdBytes() const;

    void setVolume(int percent);
    int getVolume() const;

signals:
    void playFinished(); // 播放完成信号

private slots:
    void onInternalPlayFinished();

private:
    PaStream *stream = nullptr;
    int outputDeviceIndex = -1; // 默认使用Pa_GetDefaultOutputDevice()

    int numChannels = 0;
    int sampleRate = 0;

    float m_volume = 1.0f;

    // 线程安全的缓冲区，用于存储待播放的PCM数据
    std::deque<uint8_t> m_audioBuffer;
    mutable std::mutex m_bufferMutex;
    std::condition_variable m_bufferCondVar; // 用于通知数据可用

    bool m_isStreamActive = false; // 内部状态，指示流是否正在播放
    bool m_isPausedByPlayer = false; // 内部状态，指示是否被播放器主动暂停

    // PortAudio 回调函数
    static int paCallback(const void *input,
                          void *output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData);
};

#endif // AUDIOPLAYER_H

