#include "audiodecoder.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

AudioDecoder::AudioDecoder() : m_packet(av_packet_alloc()), m_frame(av_frame_alloc()) {}
AudioDecoder::~AudioDecoder()
{
    close();
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}
bool AudioDecoder::fail(const QString &operation, int code)
{
    char message[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, message, sizeof(message));
    m_error = operation + QStringLiteral("：") + QString::fromUtf8(message);
    return false;
}
bool AudioDecoder::openFile(const QString &path)
{
    close();
    m_error.clear();
    if (!m_packet || !m_frame) return fail(QStringLiteral("分配解码缓冲失败"), AVERROR(ENOMEM));
    int result = avformat_open_input(&m_format, path.toUtf8().constData(), nullptr, nullptr);
    if (result < 0) return fail(QStringLiteral("无法打开音频"), result);
    result = avformat_find_stream_info(m_format, nullptr);
    if (result < 0) return fail(QStringLiteral("无法读取音频信息"), result);
    m_stream = av_find_best_stream(m_format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_stream < 0) return fail(QStringLiteral("文件没有可播放的音轨"), m_stream);
    const auto *parameters = m_format->streams[m_stream]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
    if (!codec) return fail(QStringLiteral("找不到解码器"), AVERROR_DECODER_NOT_FOUND);
    m_codec = avcodec_alloc_context3(codec);
    if (!m_codec) return fail(QStringLiteral("分配解码器失败"), AVERROR(ENOMEM));
    result = avcodec_parameters_to_context(m_codec, parameters);
    if (result < 0) return fail(QStringLiteral("读取编码参数失败"), result);
    result = avcodec_open2(m_codec, codec, nullptr);
    if (result < 0) return fail(QStringLiteral("打开解码器失败"), result);
    if (m_codec->sample_rate <= 0 || m_codec->ch_layout.nb_channels <= 0)
        return fail(QStringLiteral("音频参数无效"), AVERROR_INVALIDDATA);
    if (m_codec->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        const int count = m_codec->ch_layout.nb_channels;
        av_channel_layout_uninit(&m_codec->ch_layout);
        av_channel_layout_default(&m_codec->ch_layout, count);
    }
    AVChannelLayout output = AV_CHANNEL_LAYOUT_STEREO;
    result = swr_alloc_set_opts2(&m_resampler, &output, AV_SAMPLE_FMT_FLT, OutputRate,
                               &m_codec->ch_layout, m_codec->sample_fmt, m_codec->sample_rate, 0, nullptr);
    av_channel_layout_uninit(&output);
    if (result < 0) return fail(QStringLiteral("配置重采样失败"), result);
    result = swr_init(m_resampler);
    if (result < 0) return fail(QStringLiteral("初始化重采样失败"), result);
    return true;
}
bool AudioDecoder::seek(qint64 ms)
{
    if (!m_codec) return false;
    AVStream *stream = m_format->streams[m_stream];
    const qint64 start = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    const qint64 target = av_rescale_q(std::max<qint64>(0, ms), AVRational{1, 1000}, stream->time_base) + start;
    const int result = av_seek_frame(m_format, m_stream, target, AVSEEK_FLAG_BACKWARD);
    if (result < 0) return fail(QStringLiteral("无法跳转到该位置"), result);
    avcodec_flush_buffers(m_codec);
    av_packet_unref(m_packet);
    av_frame_unref(m_frame);
    swr_close(m_resampler);
    const int reset = swr_init(m_resampler);
    if (reset < 0) return fail(QStringLiteral("重置重采样失败"), reset);
    m_draining = m_decodedEnd = false;
    m_seekTarget = ms;
    return true;
}
AudioDecoder::Result AudioDecoder::decode(std::vector<float> &pcm)
{
    pcm.clear();
    if (!m_codec || !m_resampler) return Result::Error;
    for (;;) {
        if (m_decodedEnd) {
            const int capacity = std::max(256, swr_get_out_samples(m_resampler, 0));
            pcm.resize(size_t(capacity) * OutputChannels);
            uint8_t *output[] = {reinterpret_cast<uint8_t *>(pcm.data())};
            const int samples = swr_convert(m_resampler, output, capacity, nullptr, 0);
            if (samples < 0) {
                fail(QStringLiteral("排空重采样缓冲失败"), samples);
                return Result::Error;
            }
            pcm.resize(size_t(samples) * OutputChannels);
            return samples > 0 ? Result::Data : Result::End;
        }
        int result = avcodec_receive_frame(m_codec, m_frame);
        if (result == AVERROR_EOF) { m_decodedEnd = true; continue; }
        if (result == AVERROR(EAGAIN)) {
            if (m_draining) {
                fail(QStringLiteral("解码器未完成排空"), AVERROR_INVALIDDATA);
                return Result::Error;
            }
            do {
                av_packet_unref(m_packet);
                result = av_read_frame(m_format, m_packet);
            } while (result >= 0 && m_packet->stream_index != m_stream);
            if (result == AVERROR_EOF) {
                m_draining = true;
                result = avcodec_send_packet(m_codec, nullptr);
            } else if (result >= 0) {
                result = avcodec_send_packet(m_codec, m_packet);
            }
            av_packet_unref(m_packet);
            if (result < 0) {
                fail(QStringLiteral("读取音频数据失败"), result);
                return Result::Error;
            }
            continue;
        }
        if (result < 0) {
            fail(QStringLiteral("音频解码失败"), result);
            return Result::Error;
        }
        // Discard seek pre-roll before resampling, using stream timestamps and origin.
        int skip = 0;
        if (m_seekTarget >= 0 && m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            AVStream *stream = m_format->streams[m_stream];
            const qint64 start = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
            const double frameMs = (m_frame->best_effort_timestamp - start) * av_q2d(stream->time_base) * 1000.0;
            const double skipSamples = std::ceil((m_seekTarget - frameMs) * m_codec->sample_rate / 1000.0);
            skip = int(std::clamp(skipSamples, 0.0, double(m_frame->nb_samples)));
        }
        if (skip == m_frame->nb_samples) { av_frame_unref(m_frame); continue; }
        m_seekTarget = -1;
        const int planes = av_sample_fmt_is_planar(m_codec->sample_fmt) ? m_codec->ch_layout.nb_channels : 1;
        const int stride = av_get_bytes_per_sample(m_codec->sample_fmt) * (planes == 1 ? m_codec->ch_layout.nb_channels : 1);
        std::vector<const uint8_t *> input(size_t(planes), nullptr);
        for (int p = 0; p < planes; ++p) input[size_t(p)] = m_frame->extended_data[p] + skip * stride;
        const int capacity = swr_get_out_samples(m_resampler, m_frame->nb_samples - skip);
        if (capacity < 0) {
            fail(QStringLiteral("计算重采样缓冲失败"), capacity);
            av_frame_unref(m_frame);
            return Result::Error;
        }
        pcm.resize(size_t(capacity) * OutputChannels);
        uint8_t *output[] = {reinterpret_cast<uint8_t *>(pcm.data())};
        result = swr_convert(m_resampler, output, capacity, input.data(), m_frame->nb_samples - skip);
        av_frame_unref(m_frame);
        if (result < 0) { fail(QStringLiteral("重采样失败"), result); return Result::Error; }
        pcm.resize(size_t(result) * OutputChannels);
        if (!pcm.empty()) return Result::Data;
    }
}
qint64 AudioDecoder::duration() const
{
    if (!m_format || m_stream < 0) return 0;
    const auto *stream = m_format->streams[m_stream];
    if (stream->duration != AV_NOPTS_VALUE)
        return std::max<qint64>(0, av_rescale_q(stream->duration, stream->time_base, AVRational{1, 1000}));
    return m_format->duration == AV_NOPTS_VALUE ? 0 : std::max<qint64>(0, m_format->duration / 1000);
}
int AudioDecoder::sampleRate() const { return m_codec ? m_codec->sample_rate : 0; }
int AudioDecoder::channels() const { return m_codec ? m_codec->ch_layout.nb_channels : 0; }
int AudioDecoder::bitrateKbps() const
{
    return m_codec && m_codec->bit_rate > 0 ? int(m_codec->bit_rate / 1000)
                                          : (m_format ? int(m_format->bit_rate / 1000) : 0);
}
QString AudioDecoder::tag(const char *name) const
{
    if (!m_format) return {};
    const AVDictionaryEntry *entry = av_dict_get(m_format->metadata, name, nullptr, 0);
    if (!entry && m_stream >= 0) entry = av_dict_get(m_format->streams[m_stream]->metadata, name, nullptr, 0);
    return entry ? QString::fromUtf8(entry->value) : QString();
}
QString AudioDecoder::title() const { return tag("title"); }
QString AudioDecoder::artist() const { return tag("artist"); }
void AudioDecoder::close()
{
    swr_free(&m_resampler);
    avcodec_free_context(&m_codec);
    avformat_close_input(&m_format);
    if (m_packet) av_packet_unref(m_packet);
    if (m_frame) av_frame_unref(m_frame);
    m_stream = -1;
    m_draining = m_decodedEnd = false;
    m_seekTarget = -1;
}
