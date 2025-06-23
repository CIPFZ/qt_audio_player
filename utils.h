#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <QString>


struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;          // 文件大小 - 8 字节
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtChunkSize = 16; // PCM格式块大小
    uint16_t audioFormat = 1;   // PCM格式 = 1
    uint16_t numChannels;       // 声道数
    uint32_t sampleRate;        // 采样率
    uint32_t byteRate;          // 数据速率 = sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;        // 块对齐 = numChannels * bitsPerSample/8
    uint16_t bitsPerSample = 16;// 采样位数
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;          // PCM数据大小
};

class Utils
{
public:
    Utils();
    bool savePCMToWAV(const std::string &filePath, const std::vector<uint8_t> &pcmData, int sampleRate, int channels);
    std::vector<float> waveExtract(const std::vector<uint8_t> &pcmData, int channels, int samplesPerPoint = 256);
    QString formatTime(qint64 ms);
};

#endif // UTILS_H
