#include "playlistmodel.h"
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFont>

int PlaylistModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : int(m_tracks.size()); }
int PlaylistModel::columnCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : ColumnCount; }
QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const auto &track = m_tracks.at(index.row());
    const QFileInfo info(track.path);
    switch (role) {
    case PathRole: return track.path;
    case TitleRole: return track.title;
    case ArtistRole: return track.artist;
    case FolderRole: return info.dir().dirName();
    case FormatRole: return info.suffix().toUpper();
    case NumberRole: return index.row() + 1;
    case CurrentRole: return track.path == m_currentPath;
    case SelectedRole: return m_selected.contains(track.path);
    default: break;
    }
    if (role == Qt::UserRole || role == Qt::ToolTipRole) return track.path;
    if (role == Qt::ForegroundRole) return QColor(track.path == m_currentPath ? "#b8e986" : (index.column() == Title ? "#f0f1f3" : "#9396a2"));
    if (role == Qt::FontRole && index.column() == Title) { QFont font; font.setBold(track.path == m_currentPath); return font; }
    if (role == Qt::TextAlignmentRole && (index.column() == Number || index.column() == Format)) return int(Qt::AlignCenter);
    if (role != Qt::DisplayRole) return {};
    switch (index.column()) {
    case Number: return track.path == m_currentPath ? QStringLiteral("♪") : QStringLiteral("%1").arg(index.row() + 1, 2, 10, QLatin1Char('0'));
    case Title: return track.title;
    case Location: return track.artist.isEmpty() ? info.dir().dirName() : track.artist;
    case Format: return info.suffix().toUpper();
    default: return {};
    }
}
QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return {{PathRole,"filePath"},{TitleRole,"trackTitle"},{ArtistRole,"artist"},
            {FolderRole,"folder"},{FormatRole,"format"},{NumberRole,"trackNumber"},
            {CurrentRole,"isCurrent"},{SelectedRole,"isSelected"}};
}
QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    const QStringList names{QStringLiteral("#"), tr("曲目"), tr("艺术家 / 文件夹"), tr("格式")};
    return names.value(section);
}
bool PlaylistModel::isSupported(const QString &path)
{
    static const QSet<QString> extensions{"mp3", "wav", "flac", "aac", "ogg", "m4a", "opus", "aiff", "aif", "wma"};
    return extensions.contains(QFileInfo(path).suffix().toLower());
}
QString PlaylistModel::fileFilter() { return tr("音频文件 (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.opus *.aiff *.aif *.wma);;所有文件 (*)"); }
int PlaylistModel::addFiles(const QStringList &paths)
{
    QVector<Track> added;
    for (const auto &path : paths) {
        const QFileInfo info(path);
        const auto canonical = info.canonicalFilePath();
        if (!info.isFile() || !isSupported(path) || canonical.isEmpty() || m_paths.contains(canonical)) continue;
        m_paths.insert(canonical);
        added.append({canonical, info.completeBaseName(), {}});
    }
    if (added.isEmpty()) return 0;
    beginInsertRows({}, rowCount(), rowCount() + int(added.size()) - 1);
    m_tracks += added;
    endInsertRows();
    emit tracksChanged();
    return int(added.size());
}
bool PlaylistModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || row < 0 || count <= 0 || row + count > rowCount()) return false;
    beginRemoveRows({}, row, row + count - 1);
    for (int i = row; i < row + count; ++i) {
        m_paths.remove(m_tracks.at(i).path);
        m_selected.remove(m_tracks.at(i).path);
    }
    m_tracks.remove(row, count);
    endRemoveRows();
    if (row < rowCount())
        emit dataChanged(index(row,0),index(rowCount()-1,ColumnCount-1),{NumberRole,Qt::DisplayRole});
    emit tracksChanged();
    emit selectionChanged();
    return true;
}
QString PlaylistModel::pathAt(int row) const { return row >= 0 && row < rowCount() ? m_tracks.at(row).path : QString(); }
int PlaylistModel::indexOf(const QString &path) const
{
    for (int i = 0; i < rowCount(); ++i) if (m_tracks.at(i).path == path) return i;
    return -1;
}
QStringList PlaylistModel::paths() const
{
    QStringList result;
    for (const auto &track : m_tracks) result.append(track.path);
    return result;
}
void PlaylistModel::setCurrentPath(const QString &path)
{
    const int previous = indexOf(m_currentPath);
    m_currentPath = path;
    if (previous >= 0) emit dataChanged(index(previous, 0), index(previous, ColumnCount - 1));
    const int current = indexOf(path);
    if (current >= 0) emit dataChanged(index(current, 0), index(current, ColumnCount - 1));
}
void PlaylistModel::updateMetadata(const QString &path, const QString &title, const QString &artist)
{
    const int row = indexOf(path);
    if (row < 0) return;
    m_tracks[row].title = title;
    m_tracks[row].artist = artist;
    emit dataChanged(index(row, Title), index(row, Location));
    // QML ListView reads the roles on column zero.
    emit dataChanged(index(row, 0), index(row, 0), {TitleRole, ArtistRole});
}
void PlaylistModel::selectPath(const QString &path, bool extend)
{
    if (!extend) m_selected.clear();
    if (!path.isEmpty()) {
        if (extend && m_selected.contains(path)) m_selected.remove(path);
        else m_selected.insert(path);
    }
    if (rowCount() > 0) emit dataChanged(index(0,0), index(rowCount()-1,0), {SelectedRole});
    emit selectionChanged();
}
void PlaylistModel::clearSelection() { selectPath({}); }
