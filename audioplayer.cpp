#include "audioplayer.h"
#include <thread>
#include <chrono>
#include <QThread>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCritical() << "PortAudio 初始化失败:" << Pa_GetErrorText(err);
    }
    listPortAudioDevices(); // 调试用
}

AudioPlayer::~AudioPlayer() {
    stop();
    Pa_Terminate();
    qDebug() << "PortAudio 已终止。";
}

void AudioPlayer::setOutputDevice(int deviceIndex) {
    if (deviceIndex >= 0 && deviceIndex < Pa_GetDeviceCount()) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(deviceIndex);
        if (info && info->maxOutputChannels > 0) {
            outputDeviceIndex = deviceIndex;
            qDebug() << "已选择输出设备:" << deviceIndex << ":" << info->name;
        } else {
            qWarning() << "设备无效或不支持输出:" << deviceIndex;
        }
    } else {
        qWarning() << "设备索引超出范围:" << deviceIndex;
    }
}

void AudioPlayer::listPortAudioDevices() {
    int numDevices = Pa_GetDeviceCount();
    qDebug() << "可用音频设备数:" << numDevices;

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        if (info->maxOutputChannels > 0) {
            qDebug() << "设备" << i
                     << ": 名称:" << info->name
                     << ", 最大输出通道数:" << info->maxOutputChannels
                     << ", 默认低延迟:" << info->defaultLowOutputLatency;
        }
    }

    qDebug() << "默认输出设备:" << Pa_GetDefaultOutputDevice();
}

bool AudioPlayer::start(int sampleRate, int channels) {
    stop(); // 确保停止任何现有流

    this->numChannels = channels;
    this->sampleRate = sampleRate;

    PaStreamParameters outputParams;
    outputParams.device = (outputDeviceIndex >= 0) ? outputDeviceIndex : Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        qCritical() << "无可用默认输出设备，或指定设备无效。";
        return false;
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(outputParams.device);
    if (!deviceInfo) {
        qCritical() << "无法获取设备信息。";
        return false;
    }

    // 确保通道数不超过设备支持的最大值
    outputParams.channelCount = qMin(channels, deviceInfo->maxOutputChannels);
    if (outputParams.channelCount == 0) {
        qCritical() << "设备不支持任何输出通道。";
        return false;
    }
    if (outputParams.channelCount < channels) {
        qWarning() << "设备不支持" << channels << "个输出通道，将使用" << outputParams.channelCount << "个通道。";
    }

    outputParams.sampleFormat = paInt16; // PCM格式为S16
    outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(
        &stream,
        nullptr, // 无输入
        &outputParams,
        sampleRate,
        paFramesPerBufferUnspecified, // 由PortAudio决定缓冲区大小
        paNoFlag,
        paCallback,
        this);

    if (err != paNoError) {
        qCritical() << "打开 PortAudio 流失败:" << Pa_GetErrorText(err);
        stream = nullptr; // 确保stream为nullptr
        return false;
    }

    // 清空缓冲区并启动流
    clearPcmData();
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        qCritical() << "启动 PortAudio 流失败:" << Pa_GetErrorText(err);
        Pa_CloseStream(stream);
        stream = nullptr;
        return false;
    }
    m_isStreamActive = true;
    m_isPausedByPlayer = false;
    qDebug() << "PortAudio 流已启动。采样率:" << sampleRate << ", 通道:" << outputParams.channelCount;
    return true;
}

void AudioPlayer::stop() {
    if (stream) {
        PaError err = Pa_StopStream(stream); // 停止流，等待回调完成所有数据
        if (err != paNoError && err != paStreamIsNotStopped) {
            qWarning() << "停止 PortAudio 流失败:" << Pa_GetErrorText(err);
        }
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            qWarning() << "关闭 PortAudio 流失败:" << Pa_GetErrorText(err);
        }
        stream = nullptr;
    }
    clearPcmData(); // 清空所有缓冲区数据
    m_isStreamActive = false;
    m_isPausedByPlayer = false;
    qDebug() << "PortAudio 流已停止。";
}

void AudioPlayer::pause() {
    if (stream && m_isStreamActive) {
        PaError err = Pa_StopStream(stream); // 暂停流
        if (err == paNoError) {
            m_isStreamActive = false;
            m_isPausedByPlayer = true;
            qDebug() << "PortAudio 流已暂停。";
        } else {
            qWarning() << "暂停 PortAudio 流失败:" << Pa_GetErrorText(err);
        }
    }
}

bool AudioPlayer::resume() {
    if (stream && !m_isStreamActive && m_isPausedByPlayer) {
        PaError err = Pa_StartStream(stream); // 恢复流
        if (err == paNoError) {
            m_isStreamActive = true;
            m_isPausedByPlayer = false;
            qDebug() << "PortAudio 流已恢复。";
            return true;
        } else {
            qWarning() << "恢复 PortAudio 流失败:" << Pa_GetErrorText(err);
            return false;
        }
    }
    return false; // 不在暂停状态
}

bool AudioPlayer::isPlaying() const {
    if (stream) {
        PaError err = Pa_IsStreamActive(stream);
        return (err == 1); // 1表示流活跃
    }
    return false;
}


void AudioPlayer::addPcmData(const uint8_t *data, size_t size) {
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    m_audioBuffer.insert(m_audioBuffer.end(), data, data + size);
    lock.unlock();
    m_bufferCondVar.notify_one(); // 通知等待者有新数据
}

void AudioPlayer::clearPcmData() {
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    m_audioBuffer.clear();
    lock.unlock();
    m_bufferCondVar.notify_one(); // 通知等待者缓冲区已清空
}

size_t AudioPlayer::getBufferedBytes() const {
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    return m_audioBuffer.size();
}


size_t AudioPlayer::getBufferThresholdBytes() const {
    // 这是一个经验值，用于指示缓冲区数据量不足，需要生产者填充更多数据
    // 例如：200ms 的音频数据量
    return static_cast<size_t>(sampleRate * numChannels * 2 * 0.2); // 200毫秒的 S16 数据
}

void AudioPlayer::setVolume(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_volume = percent / 100.0f;
}

int AudioPlayer::getVolume() const {
    return static_cast<int>(m_volume * 100.0f);
}

void AudioPlayer::onInternalPlayFinished() {
    emit playFinished();
}

int AudioPlayer::paCallback(const void * /*input*/,
                            void *output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* /*timeInfo*/,
                            PaStreamCallbackFlags /*statusFlags*/,
                            void *userData) {
    auto *player = static_cast<AudioPlayer *>(userData);
    size_t bytesPerFrame = player->numChannels * 2; // S16_LE = 2 bytes per sample
    size_t bytesNeeded = frameCount * bytesPerFrame;
    size_t bytesCopied = 0;

    std::unique_lock<std::mutex> lock(player->m_bufferMutex);

    // 从缓冲区中复制数据
    while (bytesCopied < bytesNeeded && !player->m_audioBuffer.empty()) {
        size_t chunkToCopy = std::min(bytesNeeded - bytesCopied, player->m_audioBuffer.size());

        // 使用 std::copy_n 复制 deque 数据到输出缓冲区
        auto it = player->m_audioBuffer.begin();
//        std::copy_n(it, chunkToCopy, static_cast<uint8_t *>(output) + bytesCopied);
//        player->m_audioBuffer.erase(it, it + chunkToCopy);

        uint8_t *outPtr = static_cast<uint8_t *>(output) + bytesCopied;
        for (size_t i = 0; i < chunkToCopy; i += 2) {
            int16_t sample = int16_t((player->m_audioBuffer[i] & 0xFF) | (player->m_audioBuffer[i+1] << 8));
            float scaled = sample * player->m_volume;
            int16_t scaledSample = clamp(static_cast<int>(scaled), -32768, 32767);
            outPtr[i] = scaledSample & 0xFF;
            outPtr[i+1] = (scaledSample >> 8) & 0xFF;
        }
        player->m_audioBuffer.erase(it, it + chunkToCopy);

        bytesCopied += chunkToCopy;
    }

    lock.unlock(); // 尽早释放锁

    // 如果缓冲区数据不足，用静音填充剩余部分
    if (bytesCopied < bytesNeeded) {
        std::memset(static_cast<uint8_t *>(output) + bytesCopied, 0, bytesNeeded - bytesCopied);

        // 若缓冲区完全空了，可以通知流终止（但此时是否真正结束由上层判断）
        if (bytesCopied == 0) {
            QMetaObject::invokeMethod(player, "onInternalPlayFinished", Qt::QueuedConnection);
            return paComplete;
        }
    }

    return paContinue;
}

