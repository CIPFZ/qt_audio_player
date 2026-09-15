#include "librarycontroller.h"
#include "libraryimporter.h"
#include "playliststore.h"
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>

LibraryController::LibraryController(QObject *parent)
    : QObject(parent), m_model(this), m_proxy(this), m_player(&m_model, this), m_scanner(this),
      m_cancelled(std::make_shared<std::atomic_bool>(false)),
      m_title(tr("还没有选择音乐")), m_artist(tr("从你的音乐库开始")),
      m_audioInfo(tr("本地音频 · 用心聆听每一首"))
{
    m_playlistPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/playlist.json";
    m_proxy.setSourceModel(&m_model);
    m_proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy.setFilterKeyColumn(-1);
    connect(&m_model, &PlaylistModel::tracksChanged, this, [this] { emit collectionChanged(); m_saveTimer.start(); });
    connect(&m_model, &PlaylistModel::selectionChanged, this, &LibraryController::selectionChanged);
    connect(&m_proxy, &QAbstractItemModel::rowsInserted, this, &LibraryController::collectionChanged);
    connect(&m_proxy, &QAbstractItemModel::rowsRemoved, this, &LibraryController::collectionChanged);
    connect(&m_proxy, &QAbstractItemModel::modelReset, this, &LibraryController::collectionChanged);
    m_saveTimer.setSingleShot(true); m_saveTimer.setInterval(300);
    connect(&m_saveTimer, &QTimer::timeout, this, &LibraryController::savePlaylist);
    connect(&m_player, &PlayerController::stateChanged, this, &LibraryController::playbackChanged);
    connect(&m_player, &PlayerController::positionChanged, this, [this](qint64 ms) { m_position = ms; emit positionChanged(); });
    connect(&m_player, &PlayerController::durationChanged, this, [this](qint64 ms) {
        m_duration = ms; emit durationChanged(); emit playbackChanged();
    });
    connect(&m_player, &PlayerController::songChanged, this,
            [this](const QString &title, const QString &artist, int rate, int channels, int bitrate) {
        m_title = title;
        m_artist = artist.isEmpty() ? tr("本地音乐") : artist;
        m_audioInfo = rate > 0 ? tr("%1 Hz   ·   %2 声道   ·   %3").arg(rate).arg(channels)
                                .arg(bitrate > 0 ? tr("%1 kbps").arg(bitrate) : tr("码率未知"))
                              : tr("本地音频 · 用心聆听每一首");
        emit trackChanged();
    });
    connect(&m_player, &PlayerController::errorOccurred, this, [this](const QString &message) { report(message,true); });
    connect(&m_player, &PlayerController::modeChanged, this, [this] {
        m_settings.setValue("playback/mode",playMode()); emit modeChanged();
    });
    connect(&m_scanner, &QFutureWatcher<QStringList>::finished, this, [this] {
        const auto files = m_scanner.result();
        const int added = m_model.addFiles(files);
        report(tr("已添加 %1 首音乐%2").arg(added).arg(files.size() > added ? tr("，已跳过重复项") : QString()));
        if (files.isEmpty()) report(tr("没有找到支持的音频文件，请检查文件格式或文件夹内容。"));
        emit importingChanged();
        if (!m_pendingImports.isEmpty()) {
            const auto pending = m_pendingImports; m_pendingImports.clear(); importPaths(pending);
        }
    });
    m_volume = std::clamp(m_settings.value("playback/volume",50).toInt(),0,100);
    m_lastVolume = std::clamp(m_settings.value("playback/lastVolume",50).toInt(),1,100);
    m_player.setVolume(m_volume);
    m_player.setPlayMode(PlayMode(std::clamp(m_settings.value("playback/mode",0).toInt(),0,3)));
    dismissMessage();
    loadPlaylist();
}
LibraryController::~LibraryController()
{
    m_cancelled->store(true);
    m_scanner.waitForFinished();
    m_saveTimer.stop();
    savePlaylist();
    m_settings.sync();
}
void LibraryController::loadPlaylist()
{
    QStringList files;
    QString error;
    if (!PlaylistStore::load(m_playlistPath,files,error)) {
        // Preserve an invalid original before allowing a fresh playlist to be saved.
        const auto backup = m_playlistPath + ".broken-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz");
        m_canSave = QFile::copy(m_playlistPath,backup);
        report(error + (m_canSave ? tr("；备份位于 %1").arg(backup) : tr("；暂未保存新列表，避免覆盖原文件。")),true);
        return;
    }
    const int loaded = m_model.addFiles(files);
    if (loaded < files.size()) report(tr("已恢复 %1 首音乐，跳过 %2 个重复或失效路径。").arg(loaded).arg(files.size()-loaded));
}
void LibraryController::savePlaylist()
{
    if (!m_canSave) return;
    QString error;
    if (!PlaylistStore::save(m_playlistPath,m_model.paths(),error)) report(tr("保存播放列表失败：") + error,true);
}
void LibraryController::report(const QString &text, bool error)
{
    m_status = text; m_statusError = error; emit statusChanged();
}
void LibraryController::dismissMessage() { report(tr("支持 MP3 / FLAC / WAV / AAC / OGG 等本地音频")); }
void LibraryController::setSearchText(const QString &text)
{
    if (m_search == text) return;
    m_search = text;
    m_model.clearSelection();
    m_proxy.setFilterFixedString(text.trimmed());
    emit searchTextChanged(); emit collectionChanged();
}
QString LibraryController::stateText() const
{
    switch (m_player.state()) {
    case PlaybackState::Stopped: return tr("等待播放");
    case PlaybackState::Loading: return tr("正在载入");
    case PlaybackState::Playing: return tr("正在播放");
    case PlaybackState::Paused: return tr("已暂停");
    }
    return {};
}
void LibraryController::addFiles()
{
    const auto files = QFileDialog::getOpenFileNames(nullptr,tr("添加音乐"),
        m_settings.value("library/lastDirectory",QStandardPaths::writableLocation(QStandardPaths::MusicLocation)).toString(),PlaylistModel::fileFilter());
    if (files.isEmpty()) return;
    m_settings.setValue("library/lastDirectory",QFileInfo(files.first()).absolutePath());
    importPaths(files);
}
void LibraryController::addFolder()
{
    const auto folder = QFileDialog::getExistingDirectory(nullptr,tr("导入音乐文件夹（含子文件夹）"),
                                                         m_settings.value("library/lastDirectory").toString());
    if (folder.isEmpty()) return;
    m_settings.setValue("library/lastDirectory",folder);
    importPaths({folder});
}
void LibraryController::importUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    for (const auto &url : urls) if (url.isLocalFile()) paths.append(url.toLocalFile());
    if (!paths.isEmpty()) importPaths(paths);
}
void LibraryController::importPaths(const QStringList &paths)
{
    if (paths.isEmpty()) return;
    if (m_scanner.isRunning()) { m_pendingImports.append(paths); return; }
    report(tr("正在扫描音频文件…"));
    m_scanner.setFuture(QtConcurrent::run(scanAudioPaths,paths,m_cancelled));
    emit importingChanged();
}
void LibraryController::playRow(int row)
{
    const auto source = m_proxy.mapToSource(m_proxy.index(row,0));
    if (source.isValid()) { dismissMessage(); m_player.play(source.row()); }
}
void LibraryController::selectRow(int row, bool extend)
{
    const auto source = m_proxy.mapToSource(m_proxy.index(row,0));
    if (source.isValid()) m_model.selectPath(m_model.pathAt(source.row()),extend);
}
void LibraryController::removeSelected()
{
    auto selected = m_model.selectedPaths();
    for (int row = m_model.rowCount()-1; row >= 0; --row)
        if (selected.contains(m_model.pathAt(row))) m_model.removeRow(row);
    if (!selected.isEmpty()) report(tr("已从列表移除 %1 首音乐，原文件保留。 ").arg(selected.size()));
}
void LibraryController::togglePlayback()
{
    int selected = -1;
    const auto paths = m_model.selectedPaths();
    if (!paths.isEmpty()) selected = m_model.indexOf(paths.first());
    if (m_player.state() == PlaybackState::Stopped) dismissMessage();
    m_player.toggle(selected);
}
void LibraryController::setVolume(int value)
{
    value = std::clamp(value,0,100);
    if (value == m_volume) return;
    m_volume = value;
    if (value > 0) m_lastVolume = value;
    m_player.setVolume(value);
    m_settings.setValue("playback/volume",value);
    m_settings.setValue("playback/lastVolume",m_lastVolume);
    emit volumeChanged();
}
void LibraryController::toggleMute() { setVolume(m_volume == 0 ? m_lastVolume : 0); }
void LibraryController::cycleMode() { m_player.setPlayMode(PlayMode((playMode()+1)%4)); }
int LibraryController::savedWidth() const { return std::clamp(m_settings.value("window/width",1160).toInt(),880,2400); }
int LibraryController::savedHeight() const { return std::clamp(m_settings.value("window/height",780).toInt(),640,1600); }
void LibraryController::saveWindowSize(int width, int height)
{
    m_settings.setValue("window/width",width); m_settings.setValue("window/height",height);
}
void LibraryController::revealCurrent()
{
    const auto source = m_model.index(m_player.currentIndex(),0);
    if (!source.isValid()) return;
    setSearchText({});
    const auto proxy = m_proxy.mapFromSource(source);
    if (proxy.isValid()) { selectRow(proxy.row()); emit scrollToRow(proxy.row()); }
}
QString LibraryController::formatTime(qint64 ms) const
{
    const qint64 seconds = std::max<qint64>(0,ms)/1000;
    if (seconds >= 3600) return QStringLiteral("%1:%2:%3").arg(seconds/3600).arg(seconds/60%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(seconds/60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
}
