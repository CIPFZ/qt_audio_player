#pragma once
#include <QStringList>
namespace PlaylistStore {
bool load(const QString &fileName, QStringList &paths, QString &error);
bool save(const QString &fileName, const QStringList &paths, QString &error);
}
