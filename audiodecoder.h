#pragma once
#include <QString>
#include <vector>
struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct AVPacket;
struct AVFrame;

// Exclusively owned by the playback thread. Output: float32 stereo, 48 kHz.
class AudioDecoder
{
public:
    enum class Result { Data, End, Error };
    AudioDecoder();
    ~AudioDecoder();
    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder &operator=(const AudioDecoder &) = delete;
    bool openFile(const QString &path);
    bool seek(qint64 ms);
    Result decode(std::vector<float> &pcm);
    void close();
    QString errorString() const { return m_error; }
    QString title() const;
    QString artist() const;
    qint64 duration() const;
    int sampleRate() const;
    int channels() const;
    int bitrateKbps() const;
    static constexpr int OutputRate = 48000;
    static constexpr int OutputChannels = 2;
private:
    bool fail(const QString &operation, int code);
    QString tag(const char *name) const;
    AVFormatContext *m_format = nullptr;
    AVCodecContext *m_codec = nullptr;
    SwrContext *m_resampler = nullptr;
    AVPacket *m_packet = nullptr;
    AVFrame *m_frame = nullptr;
    int m_stream = -1;
    bool m_draining = false;
    bool m_decodedEnd = false;
    qint64 m_seekTarget = -1;
    QString m_error;
};
