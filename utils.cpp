#include "utils.h"

Utils::Utils() {}

bool Utils::savePCMToWAV(const std::string &filePath, const std::vector<uint8_t> &pcmData, int sampleRate, int channels) {
    WAVHeader header;
    header.numChannels = static_cast<uint16_t>(channels);
    header.sampleRate = sampleRate;
    header.bitsPerSample = 16;
    header.byteRate = sampleRate * channels * header.bitsPerSample / 8;
    header.blockAlign = channels * header.bitsPerSample / 8;
    header.dataSize = static_cast<uint32_t>(pcmData.size());
    header.fileSize = header.dataSize + sizeof(WAVHeader) - 8;

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) return false;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<const char*>(pcmData.data()), pcmData.size());
    outFile.close();

    return true;
}


std::vector<float> Utils::waveExtract(const std::vector<uint8_t> &pcmData, int channels, int samplesPerPoint) {
    std::vector<float> waveform;

    const int16_t *samples = reinterpret_cast<const int16_t *>(pcmData.data());
    size_t totalSamples = pcmData.size() / sizeof(int16_t);
    size_t numFrames = totalSamples / channels;

    for (size_t i = 0; i < numFrames; i += samplesPerPoint) {
        float peak = 0.0f;
        size_t end = std::min(i + samplesPerPoint, numFrames);

        for (size_t j = i; j < end; ++j) {
            for (int ch = 0; ch < channels; ++ch) {
                int16_t sample = samples[j * channels + ch];
                peak = std::max(peak, std::abs(sample / 32768.0f));
            }
        }
        waveform.push_back(peak);
    }

    return waveform;
}

QString Utils::formatTime(qint64 ms) {
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
