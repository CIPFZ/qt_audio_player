#pragma once
#include "playercontroller.h"
#include <QFutureWatcher>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <memory>

// GUI-thread API exposed to QML. PlaybackWorker owns all audio work on its own thread.
class LibraryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* playlist READ playlist CONSTANT)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY collectionChanged)
    Q_PROPERTY(int visibleCount READ visibleCount NOTIFY collectionChanged)
    Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString title READ title NOTIFY trackChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY trackChanged)
    Q_PROPERTY(QString audioInfo READ audioInfo NOTIFY trackChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY playbackChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playbackChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY playbackChanged)
    Q_PROPERTY(bool canSeek READ canSeek NOTIFY playbackChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int playMode READ playMode NOTIFY modeChanged)
    Q_PROPERTY(QString modeText READ modeText NOTIFY modeChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool statusError READ statusError NOTIFY statusChanged)
    Q_PROPERTY(int savedWidth READ savedWidth CONSTANT)
    Q_PROPERTY(int savedHeight READ savedHeight CONSTANT)
public:
    explicit LibraryController(QObject *parent = nullptr);
    ~LibraryController() override;
    QAbstractItemModel *playlist() { return &m_proxy; }
    int trackCount() const { return m_model.rowCount(); }
    int visibleCount() const { return m_proxy.rowCount(); }
    int selectionCount() const { return m_model.selectedPaths().size(); }
    QString searchText() const { return m_search; }
    void setSearchText(const QString &text);
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString audioInfo() const { return m_audioInfo; }
    QString stateText() const;
    bool playing() const { return m_player.state() == PlaybackState::Playing; }
    bool loading() const { return m_player.state() == PlaybackState::Loading; }
    bool canSeek() const { return (playing() || m_player.state() == PlaybackState::Paused) && duration() > 0; }
    qint64 duration() const { return m_duration; }
    qint64 position() const { return m_position; }
    int volume() const { return m_volume; }
    void setVolume(int value);
    int playMode() const { return int(m_player.playMode()); }
    QString modeText() const { return PlayerController::modeName(m_player.playMode()); }
    bool importing() const { return m_scanner.isRunning(); }
    QString statusMessage() const { return m_status; }
    bool statusError() const { return m_statusError; }
    int savedWidth() const;
    int savedHeight() const;
    void importPaths(const QStringList &paths);
    Q_INVOKABLE void addFiles();
    Q_INVOKABLE void addFolder();
    Q_INVOKABLE void importUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void playRow(int proxyRow);
    Q_INVOKABLE void selectRow(int proxyRow, bool extend = false);
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void next() { m_player.next(); }
    Q_INVOKABLE void previous() { m_player.previous(); }
    Q_INVOKABLE void seek(qint64 ms) { m_player.seek(ms); }
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void cycleMode();
    Q_INVOKABLE void saveWindowSize(int width, int height);
    Q_INVOKABLE void revealCurrent();
    Q_INVOKABLE QString formatTime(qint64 ms) const;
    Q_INVOKABLE void dismissMessage();
signals:
    void collectionChanged();
    void selectionChanged();
    void searchTextChanged();
    void trackChanged();
    void playbackChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void modeChanged();
    void importingChanged();
    void statusChanged();
    void scrollToRow(int row);
private:
    void report(const QString &text, bool error = false);
    void savePlaylist();
    void loadPlaylist();
    QSettings m_settings;
    PlaylistModel m_model;
    QSortFilterProxyModel m_proxy;
    PlayerController m_player;
    QFutureWatcher<QStringList> m_scanner;
    QTimer m_saveTimer;
    std::shared_ptr<std::atomic_bool> m_cancelled;
    QStringList m_pendingImports;
    QString m_playlistPath;
    bool m_canSave = true;
    QString m_search;
    QString m_title;
    QString m_artist;
    QString m_audioInfo;
    QString m_status;
    bool m_statusError = false;
    qint64 m_duration = 0;
    qint64 m_position = 0;
    int m_volume = 50;
    int m_lastVolume = 50;
};
