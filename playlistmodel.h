#pragma once
#include <QAbstractTableModel>
#include <QSet>
#include <QStringList>
#include <QVector>

struct Track
{
    QString path;
    QString title;
    QString artist;
};
class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column { Number, Title, Location, Format, ColumnCount };
    enum Role { PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, FolderRole, FormatRole, NumberRole, CurrentRole, SelectedRole };
    explicit PlaylistModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;
    int addFiles(const QStringList &paths);
    QString pathAt(int row) const;
    int indexOf(const QString &path) const;
    QStringList paths() const;
    void setCurrentPath(const QString &path);
    void updateMetadata(const QString &path, const QString &title, const QString &artist);
    void selectPath(const QString &path, bool extend = false);
    QStringList selectedPaths() const { return m_selected.values(); }
    void clearSelection();
    static bool isSupported(const QString &path);
    static QString fileFilter();
signals:
    void tracksChanged();
    void selectionChanged();
private:
    QVector<Track> m_tracks;
    QSet<QString> m_paths;
    QString m_currentPath;
    QSet<QString> m_selected;
};
