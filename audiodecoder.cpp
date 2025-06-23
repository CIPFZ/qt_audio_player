#include "audiodecoder.h"
#include <QDebug>

//AudioDecoder::AudioDecoder() {
//    initFFmpeg();
//}

//AudioDecoder::~AudioDecoder() {
//    cleanup();
//}

//void AudioDecoder::initFFmpeg() {
//    avdevice_register_all();
//    avformat_network_init();
//}

//bool AudioDecoder::openFile(const QString &filePath) {
//    cleanup();

//    if (avformat_open_input(&formatCtx, filePath.toUtf8().data(), nullptr, nullptr) != 0)
//        return false;
//    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
//        return false;

//    audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
//    if (audioStreamIndex < 0)
//        return false;

//    AVStream *stream = formatCtx->streams[audioStreamIndex];
//    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
//    if (!codec)
//        return false;

//    codecCtx = avcodec_alloc_context3(codec);
//    avcodec_parameters_to_context(codecCtx, stream->codecpar);

//    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
//        return false;

//    // ==== 适配 FFmpeg 6：创建输入输出 layout ====
//    AVChannelLayout in_ch_layout = codecCtx->ch_layout;
//    AVChannelLayout out_ch_layout;
//    av_channel_layout_default(&out_ch_layout, 2);  // 输出强制为立体声

//    // ==== 初始化 SwrContext ====
//    int err = swr_alloc_set_opts2(
//        &swrCtx,
//        &out_ch_layout, AV_SAMPLE_FMT_S16, codecCtx->sample_rate,
//        &in_ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
//        0, nullptr);

//    if (err < 0 || !swrCtx || swr_init(swrCtx) < 0) {
//        qDebug() << "Failed to init swr context";
//        av_channel_layout_uninit(&out_ch_layout);
//        return false;
//    }

//    av_channel_layout_uninit(&out_ch_layout);
//    return true;
//}

//bool AudioDecoder::decodeAll(std::vector<uint8_t> &outPCM, int &sampleRate, int &channels) {
//    if (!codecCtx || !swrCtx)
//        return false;

//    sampleRate = codecCtx->sample_rate;
//    channels = 2;  // 输出固定为立体声

//    AVPacket *packet = av_packet_alloc();
//    AVFrame *frame = av_frame_alloc();

//    while (av_read_frame(formatCtx, packet) >= 0) {
//        if (packet->stream_index != audioStreamIndex) {
//            av_packet_unref(packet);
//            continue;
//        }

//        if (avcodec_send_packet(codecCtx, packet) != 0) {
//            av_packet_unref(packet);
//            continue;
//        }

//        while (avcodec_receive_frame(codecCtx, frame) == 0) {
//            int outSamples = av_rescale_rnd(
//                swr_get_delay(swrCtx, sampleRate) + frame->nb_samples,
//                sampleRate, sampleRate, AV_ROUND_UP);

//            std::vector<uint8_t> tempBuffer(outSamples * channels * 2);  // s16 = 2 bytes/sample
//            uint8_t *outPtrs[] = { tempBuffer.data() };

//            int converted = swr_convert(
//                swrCtx, outPtrs, outSamples,
//                (const uint8_t **)frame->data, frame->nb_samples);

//            if (converted > 0) {
//                int dataSize = converted * channels * 2;
//                outPCM.insert(outPCM.end(), tempBuffer.begin(), tempBuffer.begin() + dataSize);
//            }
//        }
//        av_packet_unref(packet);
//    }

//    av_frame_free(&frame);
//    av_packet_free(&packet);
//    return true;
//}

//void AudioDecoder::cleanup() {
//    if (swrCtx) {
//        swr_free(&swrCtx);
//        swrCtx = nullptr;
//    }
//    if (codecCtx) {
//        avcodec_free_context(&codecCtx);
//        codecCtx = nullptr;
//    }
//    if (formatCtx) {
//        avformat_close_input(&formatCtx);
//        formatCtx = nullptr;
//    }
//}


AudioDecoder::AudioDecoder() {
    initFFmpeg();
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
}

AudioDecoder::~AudioDecoder() {
    cleanup();
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

void AudioDecoder::initFFmpeg() {
    avformat_network_init(); // 如果需要网络流，保留
    qDebug() << "FFmpeg initialized.";
}

bool AudioDecoder::openFile(const QString &filePath) {
    cleanup();

    // 打开输入文件
    if (avformat_open_input(&formatCtx, filePath.toUtf8().data(), nullptr, nullptr) != 0) {
        qDebug() << "无法打开文件:" << filePath;
        return false;
    }

    // 查找流信息
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        qDebug() << "无法查找流信息";
        return false;
    }

    // 查找最佳音频流
    audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0) {
        qDebug() << "未找到音频流";
        return false;
    }

    AVStream *stream = formatCtx->streams[audioStreamIndex];
    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qDebug() << "未找到解码器:" << avcodec_get_name(stream->codecpar->codec_id);
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qDebug() << "无法分配解码器上下文";
        return false;
    }

    if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0) {
        qDebug() << "无法将参数复制到解码器上下文";
        return false;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        qDebug() << "无法打开解码器";
        return false;
    }

    // 适配 FFmpeg 6：创建输入输出 channel layout
    AVChannelLayout in_ch_layout = codecCtx->ch_layout;
    AVChannelLayout out_ch_layout;
    // 目标输出格式：立体声，16位带符号整数，采样率与源相同
    av_channel_layout_default(&out_ch_layout, 2); // 输出强制为立体声
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;

    // 初始化 SwrContext
    int err = swr_alloc_set_opts2(
        &swrCtx,
        &out_ch_layout, out_sample_fmt, codecCtx->sample_rate,
        &in_ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
        0, nullptr);

    if (err < 0 || !swrCtx || swr_init(swrCtx) < 0) {
        qDebug() << "初始化 SwrContext 失败:" << err;
        // 确保清理 out_ch_layout
        if (out_ch_layout.u.mask != 0 || out_ch_layout.nb_channels != 0) { // 检查是否已初始化
             av_channel_layout_uninit(&out_ch_layout);
        }
        return false;
    }

    // 存储最终输出的采样率和通道数
    m_sampleRate = codecCtx->sample_rate;
    m_channels = 2; // SwrContext 输出强制为立体声

    // 清理 out_ch_layout，因为 swrCtx 内部已经复制了
    if (out_ch_layout.u.mask != 0 || out_ch_layout.nb_channels != 0) {
        av_channel_layout_uninit(&out_ch_layout);
    }
    qDebug() << "文件打开成功，采样率:" << m_sampleRate << ", 通道数:" << m_channels;
    return true;
}

qint64 AudioDecoder::getDurationMs() const {
    if (formatCtx && formatCtx->duration != AV_NOPTS_VALUE) {
        // formatCtx->duration 的单位是 AV_TIME_BASE，通常是微秒
        return static_cast<qint64>(formatCtx->duration * 1000 / AV_TIME_BASE);
    }
    return 0;
}

bool AudioDecoder::seek(qint64 ms) {
    if (!formatCtx || audioStreamIndex < 0) {
        return false;
    }

    // 将毫秒转换为 FFmpeg 的时间戳单位
    int64_t target_ts = static_cast<int64_t>(ms / 1000.0 * AV_TIME_BASE); // 微秒
    target_ts = av_rescale_q(target_ts, {1, AV_TIME_BASE}, formatCtx->streams[audioStreamIndex]->time_base);


    if (av_seek_frame(formatCtx, audioStreamIndex, target_ts, AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << "Seek failed to:" << ms << "ms";
        return false;
    }

    // 清空解码器缓冲区
    avcodec_flush_buffers(codecCtx);
    qDebug() << "Seek successful to:" << ms << "ms";
    return true;
}

// 辅助函数：读取并解码一个帧
bool AudioDecoder::readAndDecodeFrame() {
    int ret = 0;
    while (true) {
        // 尝试接收已解码的帧
        ret = avcodec_receive_frame(codecCtx, m_frame);
        if (ret == 0) {
            // 成功接收到帧，返回true
            return true;
        } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // 需要更多数据包或已到达文件末尾
            // 如果是因为需要更多数据包，则读取新包
            if (ret == AVERROR(EAGAIN)) {
                av_packet_unref(m_packet); // 释放旧包
                ret = av_read_frame(formatCtx, m_packet);
                if (ret < 0) { // 无法读取更多包 (文件结束或错误)
                    if (ret != AVERROR_EOF) {
                        qDebug() << "读取数据包失败或文件结束:" << ret;
                    }
                    return false; // 文件结束或读取错误
                }
                if (m_packet->stream_index == audioStreamIndex) {
                    avcodec_send_packet(codecCtx, m_packet); // 发送新包
                } else {
                    av_packet_unref(m_packet); // 不是音频流，释放包
                    continue; // 继续读取下一个包
                }
            } else { // AVERROR_EOF，文件已读取完所有包
                return false;
            }
        } else {
            // 其他错误
            qDebug() << "解码帧失败:" << ret;
            return false;
        }
    }
}

int AudioDecoder::decodeFrame(uint8_t *outBuffer, int maxBufferSize, int &outSampleRate, int &outChannels) {
    if (!codecCtx || !swrCtx || !m_packet || !m_frame) {
        return -1; // 解码器未准备好
    }

    if (!readAndDecodeFrame()) {
        return 0; // 没有更多数据或发生错误
    }

    outSampleRate = m_sampleRate;
    outChannels = m_channels;

    int outSamples = av_rescale_rnd(
        swr_get_delay(swrCtx, m_sampleRate) + m_frame->nb_samples,
        m_sampleRate, m_sampleRate, AV_ROUND_UP);

    // 检查缓冲区大小是否足够
    int requiredSize = outSamples * outChannels * 2; // S16 = 2 bytes/sample
    if (requiredSize > maxBufferSize) {
        qWarning() << "输出缓冲区太小，需要" << requiredSize << "字节，但只有" << maxBufferSize << "字节。";
        return -1; // 缓冲区太小
    }

    uint8_t *outPtrs[] = { outBuffer };

    int converted = swr_convert(
        swrCtx, outPtrs, outSamples,
        (const uint8_t **)m_frame->data, m_frame->nb_samples);

    av_frame_unref(m_frame); // 释放帧引用

    if (converted > 0) {
        return converted * outChannels * 2; // 返回实际转换的字节数
    } else if (converted == 0) {
        return 0; // 没有转换出数据，可能是因为delay等
    } else {
        qDebug() << "swr_convert 失败:" << converted;
        return -1; // 转换失败
    }
}

int AudioDecoder::getBitrate() const {
    return formatCtx ? formatCtx->bit_rate / 1000 : 0;
}

QString AudioDecoder::getFormatName() const {
    return formatCtx && formatCtx->iformat ? formatCtx->iformat->long_name : "Unknown";
}


void AudioDecoder::cleanup() {
    if (swrCtx) {
        swr_free(&swrCtx);
        swrCtx = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }
    audioStreamIndex = -1;
    m_sampleRate = 0;
    m_channels = 0;
    // 不释放 m_packet 和 m_frame，因为它们是成员变量，可以在下次打开文件时重用
    // 但要确保它们的状态被重置
    if (m_packet) av_packet_unref(m_packet);
    if (m_frame) av_frame_unref(m_frame);
}
