#include "audioplayer.h"
#include "audiodecoder.h"
AudioPlayer::~AudioPlayer()
{
    close();
    if (m_initialized) Pa_Terminate();
}
bool AudioPlayer::check(PaError code, const QString &operation)
{
    if (code == paNoError) return true;
    m_error = operation + QStringLiteral("：") + QString::fromUtf8(Pa_GetErrorText(code));
    return false;
}
bool AudioPlayer::open()
{
    close();
    if (!m_initialized) {
        if (!check(Pa_Initialize(), QStringLiteral("初始化音频设备失败"))) return false;
        m_initialized = true;
    }
    PaStreamParameters output{};
    output.device = Pa_GetDefaultOutputDevice();
    const PaDeviceInfo *device = output.device == paNoDevice ? nullptr : Pa_GetDeviceInfo(output.device);
    if (!device) {
        m_error = QStringLiteral("没有可用的音频输出设备，请连接扬声器或耳机后重试。");
        return false;
    }
    output.channelCount = AudioDecoder::OutputChannels;
    output.sampleFormat = paFloat32;
    output.suggestedLatency = device->defaultLowOutputLatency;
    if (!check(Pa_IsFormatSupported(nullptr, &output, AudioDecoder::OutputRate),
               QStringLiteral("默认设备不支持 48 kHz 立体声输出"))) return false;
    return check(Pa_OpenStream(&m_stream, nullptr, &output, AudioDecoder::OutputRate,
                               512, paNoFlag, &AudioPlayer::callback, this), QStringLiteral("打开音频输出失败"));
}
bool AudioPlayer::start()
{
    return m_stream && check(Pa_StartStream(m_stream), QStringLiteral("开始播放失败"));
}
bool AudioPlayer::pause()
{
    return m_stream && (Pa_IsStreamStopped(m_stream) == 1 ||
                       check(Pa_StopStream(m_stream), QStringLiteral("暂停播放失败")));
}
void AudioPlayer::close()
{
    if (m_stream) {
        if (Pa_IsStreamStopped(m_stream) == 0) Pa_AbortStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
    reset();
}
void AudioPlayer::reset()
{
    m_buffer.reset();
    m_eof.store(0);
    m_framesPlayed.store(0);
}
int AudioPlayer::streamStatus() const { return m_stream ? Pa_IsStreamActive(m_stream) : paBadStreamPtr; }
bool AudioPlayer::finished() const
{
    return m_eof.load(std::memory_order_acquire) && m_buffer.available() == 0 && streamStatus() == 0;
}
qint64 AudioPlayer::positionMs() const
{
    return qint64(m_framesPlayed.load(std::memory_order_relaxed) * 1000 / AudioDecoder::OutputRate);
}
int AudioPlayer::callback(const void *, void *output, unsigned long frames,
                          const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *context)
{
    auto *player = static_cast<AudioPlayer *>(context);
    // Acquire EOF before reading PCM: the end marker must not overtake its preceding data.
    const bool eof = player->m_eof.load(std::memory_order_acquire) != 0;
    const auto samples = player->m_buffer.read(static_cast<float *>(output), uint32_t(frames * 2),
                                              player->m_volume.load(std::memory_order_relaxed) / 100.0f);
    player->m_framesPlayed.fetch_add(samples / 2, std::memory_order_relaxed);
    return eof && player->m_buffer.available() == 0 ? paComplete : paContinue;
}
