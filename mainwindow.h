#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardPaths>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include "ui_manager.h"
#include "playercontroller.h"
#include "utils.h"
#include "playlistitemwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // menu bar
    void onAddFolderClicked();
    void onExitClicked();
    void onExtraAudioClicked();
    void onTTSClicked();
    void onSTTClicked();
    void onOpenSettings();
    void onShowAbout();
    // function
    void updatePlaylistSelection(int selectedRow);
    void onAddFilesClicked();
    void onPlayPauseClicked();
    void onPrevClicked();
    void onNextClicked();
    void onLoopClicked();
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onProgressChanged();
    void onVolumeChanged(int value);
    void onPlaybackStateChanged(bool playing);
    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onAutoNextSong();

private:
    UIManager *m_uiManager;
    PlayerController *m_playerController;
    Utils *m_utils;

    QStringList m_fileList;
    PlayMode m_playMode;

    int sliderValueToMs(int sliderValue);
    int msToSliderValue(qint64 ms);

    void setupConnections();
    void savePlaylistToFile();
    void loadPlaylistFromFile();
    void addFileToPlaylist(const QString &filePath);
    void changeSongTitle(const QString &title, int sampleRate, int channels, int bitrateKbps);
};

#endif // MAINWINDOW_H
