#include "mainwindow.h"
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_uiManager = new UIManager(this);
    m_uiManager->setupUi(this);

    m_playerController = new PlayerController(this);

    m_utils = new Utils();
    m_playMode = PlayMode::Sequential;

    setupConnections();
    loadPlaylistFromFile();  // <-- 加载历史播放列表
}

MainWindow::~MainWindow()
{
    // m_uiManager 和 m_playerController 都有父对象，会自动删除
    savePlaylistToFile(); // <-- 保存播放列表
    delete m_uiManager;
    delete m_utils;
}

void MainWindow::setupConnections()
{
    // menu bar
    // file
    connect(m_uiManager->addFileAction(), &QAction::triggered, this, &MainWindow::onAddFilesClicked);
    connect(m_uiManager->addFolderAction(), &QAction::triggered, this, &MainWindow::onAddFolderClicked);
    connect(m_uiManager->exitAction(), &QAction::triggered, this, &MainWindow::onExitClicked);
    // tool
    connect(m_uiManager->extraAction(), &QAction::triggered, this, &MainWindow::onExtraAudioClicked);
    connect(m_uiManager->ttsAction(), &QAction::triggered, this, &MainWindow::onTTSClicked);
    connect(m_uiManager->sttAction(), &QAction::triggered, this, &MainWindow::onSTTClicked);
    // setting
    connect(m_uiManager->settingsAction(), &QAction::triggered, this, &MainWindow::onOpenSettings);
    // help
    connect(m_uiManager->aboutAction(), &QAction::triggered, this, &MainWindow::onShowAbout);
    // 连接 UI 交互
    connect(m_uiManager->addFilesButton(), &QPushButton::clicked, this, &MainWindow::onAddFilesClicked);
    connect(m_uiManager->playPauseButton(), &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_uiManager->prevButton(), &QPushButton::clicked, this, &MainWindow::onPrevClicked);
    connect(m_uiManager->nextButton(), &QPushButton::clicked, this, &MainWindow::onNextClicked);
    connect(m_uiManager->loopButton(), &QPushButton::clicked, this, &MainWindow::onLoopClicked);
    connect(m_uiManager->playlistWidget(), &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    connect(m_uiManager->progressBar(), &QSlider::sliderReleased, this, &MainWindow::onProgressChanged);
    connect(m_uiManager->volumeSlider(), &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);

    // 播放器状态反馈
    connect(m_playerController, &PlayerController::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_playerController, &PlayerController::positionChanged, this, &MainWindow::onPositionChanged);
    connect(m_playerController, &PlayerController::durationChanged, this, &MainWindow::onDurationChanged);
    connect(m_playerController, &PlayerController::playCompleted, this, &MainWindow::onAutoNextSong);
    connect(m_playerController, &PlayerController::currentSongChanged, this, &MainWindow::changeSongTitle);

}

void MainWindow::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, "选择音频文件", "", "音频文件 (*.mp3 *.wav *.flac *.aac *.ogg)");
    if (files.isEmpty()) return;

    for (const QString &file : files) {
        if (!m_fileList.contains(file)) {
            m_fileList.append(file);
            // 创建自定义控件
            addFileToPlaylist(file);
        }
    }
    m_playerController->setPlaylist(m_fileList);

}

void MainWindow::updatePlaylistSelection(int selectedRow)
{
    for (int i = 0; i < m_uiManager->playlistWidget()->count(); ++i) {
        auto widget = qobject_cast<PlaylistItemWidget *>(
            m_uiManager->playlistWidget()->itemWidget(m_uiManager->playlistWidget()->item(i)));
        if (widget) {
            widget->setSelected(i == selectedRow);
        }
    }
}

void MainWindow::addFileToPlaylist(const QString &filePath) {
    auto *itemWidget = new PlaylistItemWidget(QFileInfo(filePath).fileName());
    QListWidgetItem *listItem = new QListWidgetItem(m_uiManager->playlistWidget());
    listItem->setSizeHint(QSize(0, 40));

    m_uiManager->playlistWidget()->addItem(listItem);
    m_uiManager->playlistWidget()->setItemWidget(listItem, itemWidget);

    connect(itemWidget, &PlaylistItemWidget::requestDelete, this, [=]() {
        // stop now play song
        m_playerController->stop();
        int row = m_uiManager->playlistWidget()->row(listItem);
        m_fileList.removeAt(row);
        delete m_uiManager->playlistWidget()->takeItem(row);
        // 可选: 通知 playerController 更新播放列表
        m_playerController->setPlaylist(m_fileList);
        // change title
        changeSongTitle("未选择歌曲", 0, 0, 0);
        // change progress
        onPositionChanged(0);
        onDurationChanged(0);
    });
}

void MainWindow::changeSongTitle(const QString &title, int sampleRate, int channels, int bitrateKbps)
{
    m_uiManager->currentSongTitleLabel()->setText(title);
    QString info = QString("%1 Hz | %2 | %3 kbps")
                       .arg(sampleRate == 0 ? "-" : QString::number(sampleRate))
                       .arg(channels == 0 ? "-" : QString::number(channels))
                       .arg(bitrateKbps == 0 ? "-" : QString::number(bitrateKbps));
    m_uiManager->audioInfoLabel()->setText(info);
}

void MainWindow::onPlayPauseClicked() {
    if (!m_fileList.isEmpty()) {
        if (!m_playerController->isPlaying()) {
            if (m_uiManager->playlistWidget()->currentRow() < 0) {
                m_uiManager->playlistWidget()->setCurrentRow(0); // 默认第一首
            }
            int row = m_uiManager->playlistWidget()->currentRow();
            m_playerController->play(row);
            updatePlaylistSelection(row);
            updatePlaylistSelection(row);
        } else {
            m_playerController->pause();
        }
    }
}

void MainWindow::onPrevClicked() {
    int idx = m_uiManager->playlistWidget()->currentRow();
    int count = m_fileList.size();
    if (count == 0) return;

    int newIndex = (idx > 0) ? (idx - 1) : (count - 1); // 如果是第一首，就跳到最后一首
    if (m_playMode == PlayMode::Shuffle) {
        newIndex = QRandomGenerator::global()->bounded(count);
    }
    m_uiManager->playlistWidget()->setCurrentRow(newIndex);
    m_playerController->play(newIndex);
    updatePlaylistSelection(newIndex);
}

void MainWindow::onNextClicked() {
    int idx = m_uiManager->playlistWidget()->currentRow();
    int count = m_fileList.size();
    if (count == 0) return;

    int newIndex = (idx < count - 1) ? (idx + 1) : 0; // 如果是最后一首，就跳到第一首
    if (m_playMode == PlayMode::Shuffle) {
        newIndex = QRandomGenerator::global()->bounded(count);
    }
    m_uiManager->playlistWidget()->setCurrentRow(newIndex);
    m_playerController->play(newIndex);
    updatePlaylistSelection(newIndex);
}

void MainWindow::onLoopClicked() {
    // 切换模式
    switch (m_playMode) {
        case PlayMode::Sequential:
            m_playMode = PlayMode::LoopOne;
            m_uiManager->setLoopButtonIcon(":/icons/repeat_one.png");
            m_uiManager->playModeLabel()->setText("单曲循环");
            break;
        case PlayMode::LoopOne:
            m_playMode = PlayMode::Shuffle;
            m_uiManager->setLoopButtonIcon(":/icons/shuffle.png");
            m_uiManager->playModeLabel()->setText("随机播放");
            break;
        case PlayMode::Shuffle:
            m_playMode = PlayMode::Sequential;
            m_uiManager->setLoopButtonIcon(":/icons/sequential.png");
            m_uiManager->playModeLabel()->setText("顺序播放");
            break;
    }
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item) {
    int row = m_uiManager->playlistWidget()->row(item);
    m_uiManager->playlistWidget()->setCurrentRow(row);
    m_playerController->play(row);

    // 设置选中状态（高亮）样式
    updatePlaylistSelection(row);
}

void MainWindow::onProgressChanged() {
//    int sliderVal = m_uiManager->progressBar()->value();
//    qint64 targetMs = sliderVal; // 注意：需要将 value 转换为毫秒
//    m_playerController->seek(targetMs);
}

void MainWindow::onVolumeChanged(int value) {
    m_playerController->setVolume(value);
}

void MainWindow::onPlaybackStateChanged(bool playing) {
    m_uiManager->setPlayPauseButtonIcon(playing);
}

void MainWindow::onPositionChanged(qint64 ms) {
    m_uiManager->progressBar()->setValue(static_cast<int>(ms));
    m_uiManager->currentTimeLabel()->setText(m_utils->formatTime(ms));
}

void MainWindow::onDurationChanged(qint64 ms) {
    m_uiManager->progressBar()->setMaximum(static_cast<int>(ms));
    m_uiManager->totalTimeLabel()->setText(m_utils->formatTime(ms));
}

void MainWindow::onAutoNextSong()
{
    int count = m_fileList.size();
    int idx = m_uiManager->playlistWidget()->currentRow();

    switch (m_playMode) {
        case PlayMode::Sequential:
            idx = (idx + 1) % count;
            break;
        case PlayMode::LoopOne:
            // 不变，重复当前
            break;
        case PlayMode::Shuffle:
            idx = QRandomGenerator::global()->bounded(count);
            break;
    }

    m_uiManager->playlistWidget()->setCurrentRow(idx);
    m_playerController->play(idx);
    updatePlaylistSelection(idx);
}

void MainWindow::savePlaylistToFile() {
    QJsonArray array;
    for (const QString &file : m_fileList) {
        array.append(file);
    }

    QJsonDocument doc(array);
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path); // 确保路径存在
    QFile file(path + "/playlist.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadPlaylistFromFile() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/playlist.json";
    QFile file(path);
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const QJsonValue &val : array) {
                QString filePath = val.toString();
                if (QFile::exists(filePath)) { // 只加载存在的文件
                    m_fileList.append(filePath);
                    addFileToPlaylist(filePath);
                }
            }
            m_playerController->setPlaylist(m_fileList);
        }
    }
}

void MainWindow::onAddFolderClicked()
{
    QString folder = QFileDialog::getExistingDirectory(this, "选择音频文件夹");
    if (!folder.isEmpty()) {
        QDir dir(folder);
        QStringList filters = {"*.mp3", "*.wav", "*.flac", "*.aac", "*.ogg"};
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

        for (const QFileInfo &fileInfo : fileList) {
            QString filePath = fileInfo.absoluteFilePath();
            if (m_fileList.contains(filePath)) {
                continue;
            }
            m_fileList.append(filePath);
            addFileToPlaylist(filePath);
        }
        m_playerController->setPlaylist(m_fileList);
    }
}

void MainWindow::onExitClicked()
{
    qApp->quit();
}

void MainWindow::onExtraAudioClicked()
{
    QMessageBox::information(this, "提示", "音频提取功能尚未实现！");
}

void MainWindow::onTTSClicked()
{
    QMessageBox::information(this, "提示", "语音转文字（TTS）功能尚未实现！");
}

void MainWindow::onSTTClicked()
{
    QMessageBox::information(this, "提示", "语音识别（STT）功能尚未实现！");
}

void MainWindow::onOpenSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("设置");
    dialog.resize(500, 300);

    QHBoxLayout *layout = new QHBoxLayout(&dialog);
    QTabWidget *tabWidget = new QTabWidget;
    layout->addWidget(tabWidget);

    // 音频设置页
    QWidget *audioTab = new QWidget;
    QFormLayout *formLayout = new QFormLayout(audioTab);
    QComboBox *outputDeviceCombo = new QComboBox;
    outputDeviceCombo->addItems({"默认输出设备", "扬声器", "耳机"});  // 示例
    formLayout->addRow("输出设备：", outputDeviceCombo);

    tabWidget->addTab(audioTab, "音频");

    dialog.exec();
}

void MainWindow::onShowAbout()
{
    QDialog dialog(this);
    dialog.setWindowTitle("关于音乐播放器");
    dialog.setModal(true);
    dialog.setFixedSize(400, 250);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(":/icons/logo.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);

    QLabel *infoLabel = new QLabel(R"(
        <h3>音乐播放器 v1.0</h3>
        <p>开发者：CIPFZ<br>
        公司：BitZion<br>
        依赖：Qt 5.14，FFmpeg，PortAudio<br>
        版权所有 © 2025</p>
    )");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setTextFormat(Qt::RichText);

    QPushButton *closeButton = new QPushButton("Close");
    closeButton->setFixedSize(100, 32);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #66BB6A;
        }
    )");

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    layout->addWidget(logoLabel);
    layout->addWidget(infoLabel);
    layout->addSpacing(10);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);

    dialog.exec();
}


