#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QString>
#include <vector>
#include <cstdint>
#include <QDebug>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}

class AudioDecoder
{
public:
    AudioDecoder();
    ~AudioDecoder();

    // 初始化 FFmpeg 相关库
    void initFFmpeg();

    // 打开音频文件
    bool openFile(const QString &filePath);
    // 获取歌曲总时长（毫秒）
    qint64 getDurationMs() const;
    // 跳转到指定时间点（毫秒）
    bool seek(qint64 ms);

    // 解码一帧音频数据，返回解码后的PCM数据
    // 返回值：解码的字节数，0表示数据不足/结束，-1表示错误
    int decodeFrame(uint8_t *outBuffer, int maxBufferSize, int &outSampleRate, int &outChannels);

    // 获取当前解码的采样率和通道数
    int getSampleRate() const { return m_sampleRate; }
    int getChannels() const { return m_channels; }
    // 平均比特率 kbps
    int getBitrate() const;
    // 返回如 "MP3", "WAV"
    QString getFormatName() const;

    // 清理资源
    void cleanup();

private:
    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    SwrContext *swrCtx = nullptr;
    int audioStreamIndex = -1;

    AVPacket *m_packet = nullptr; // 用于读取压缩数据包
    AVFrame *m_frame = nullptr;   // 用于接收解码后的原始帧

    int m_sampleRate = 0;
    int m_channels = 0; // 解码器输出的通道数 (通常是2)

    bool readAndDecodeFrame(); // 辅助函数：读取并解码一个帧
};


#endif // AUDIODECODER_H
