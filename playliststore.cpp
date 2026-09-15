#include "playliststore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

bool PlaylistStore::load(const QString &fileName, QStringList &paths, QString &error)
{
    paths.clear();
    QFile file(fileName);
    if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly)) { error = file.errorString(); return false; }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isArray()) {
        error = QStringLiteral("播放列表格式损坏，原文件已保留：") + fileName;
        return false;
    }
    for (const auto &entry : document.array()) {
        if (!entry.isString()) { error = QStringLiteral("播放列表包含无效记录：") + fileName; paths.clear(); return false; }
        paths.append(entry.toString());
    }
    return true;
}
bool PlaylistStore::save(const QString &fileName, const QStringList &paths, QString &error)
{
    if (!QDir().mkpath(QFileInfo(fileName).absolutePath())) { error = QStringLiteral("无法创建播放列表目录"); return false; }
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
    const auto bytes = QJsonDocument(QJsonArray::fromStringList(paths)).toJson();
    if (file.write(bytes) != bytes.size() || !file.commit()) { error = file.errorString(); return false; }
    return true;
}
