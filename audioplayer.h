#pragma once
#include "pcmbuffer.h"
#include <QString>
#include <portaudio.h>

// Lifecycle is confined to the playback thread; callback() alone consumes PCM.
class AudioPlayer
{
public:
    AudioPlayer() = default;
    ~AudioPlayer();
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;
    bool open();
    bool start();
    bool pause();
    void close();
    void reset();
    uint32_t write(const float *samples, uint32_t count) { return m_buffer.write(samples, count); }
    uint32_t bufferedSamples() const { return m_buffer.available(); }
    void setVolume(int percent) { m_volume.store(uint32_t(std::clamp(percent, 0, 100))); }
    void setEndOfInput() { m_eof.store(1, std::memory_order_release); }
    bool finished() const;
    int streamStatus() const;
    qint64 positionMs() const;
    QString errorString() const { return m_error; }
private:
    bool check(PaError code, const QString &operation);
    static int callback(const void *, void *, unsigned long, const PaStreamCallbackTimeInfo *,
                        PaStreamCallbackFlags, void *);
    static_assert(std::atomic<uint64_t>::is_always_lock_free, "Audio requires lock-free frame counter");
    PaStream *m_stream = nullptr;
    bool m_initialized = false;
    PcmBuffer m_buffer;
    std::atomic<uint32_t> m_volume{50};
    std::atomic<uint32_t> m_eof{0};
    std::atomic<uint64_t> m_framesPlayed{0};
    QString m_error;
};
