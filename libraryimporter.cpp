#include "libraryimporter.h"
#include "playlistmodel.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <algorithm>

QStringList scanAudioPaths(const QStringList &inputs, const std::shared_ptr<std::atomic_bool> &cancelled)
{
    QSet<QString> found;
    auto collect = [&found](const QString &path) {
        const QFileInfo info(path);
        if (info.isFile() && PlaylistModel::isSupported(path)) {
            const auto canonical = info.canonicalFilePath();
            if (!canonical.isEmpty()) found.insert(canonical);
        }
    };
    for (const auto &input : inputs) {
        if (cancelled->load()) break;
        if (QFileInfo(input).isDir()) {
            // Do not follow directory symlinks; avoids cycles and escaping a chosen tree.
            QDirIterator iterator(input, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (iterator.hasNext() && !cancelled->load()) collect(iterator.next());
        } else collect(input);
    }
    auto result = found.values();
    std::sort(result.begin(), result.end(), [](const QString &a, const QString &b) { return QString::localeAwareCompare(a,b) < 0; });
    return result;
}
