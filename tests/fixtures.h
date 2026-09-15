#pragma once
#include <QDataStream>
#include <QFile>
#include <cmath>

inline bool writeWave(const QString &path, int durationMs = 1200, int rate = 22050, int channels = 1)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const int frames = rate * durationMs / 1000;
    const quint32 bytes = quint32(frames * channels * 2);
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF",4); out << quint32(36 + bytes); out.writeRawData("WAVEfmt ",8);
    out << quint32(16) << quint16(1) << quint16(channels) << quint32(rate) << quint32(rate*channels*2)
        << quint16(channels*2) << quint16(16);
    out.writeRawData("data",4); out << bytes;
    for (int frame=0; frame<frames; ++frame)
        for (int channel=0; channel<channels; ++channel)
            out << qint16(14000 * std::sin(frame * 2 * 3.14159265358979323846 * (440 + channel*110) / rate));
    return out.status() == QDataStream::Ok;
}
