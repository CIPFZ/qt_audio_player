#include "ui_manager.h"
// 用于设置按钮图标

UIManager::UIManager(QWidget *parent)
    : QObject(parent),
      m_addFileAction(nullptr),
      m_addFolderAction(nullptr),
      m_exitAction(nullptr),
      m_extractAction(nullptr),
      m_ttsAction(nullptr),
      m_sttAction(nullptr),
      m_settingsAction(nullptr),
      m_aboutAction(nullptr),
      m_currentSongTitleLabel(nullptr),
      m_playModeLabel(nullptr),
      m_audioInfoLabel(nullptr),
      m_playlistWidget(nullptr),
      m_addFilesButton(nullptr),
      m_prevButton(nullptr),
      m_playPauseButton(nullptr),
      m_nextButton(nullptr),
      m_loopButton(nullptr),
      m_progressBar(nullptr),
      m_volumeSlider(nullptr),
      m_currentTimeLabel(nullptr),
      m_totalTimeLabel(nullptr),
      m_deleteButton(nullptr),
      m_settingsButton(nullptr)
{
    // 构造函数中不进行 UI 初始化，留给 setupUi
}

UIManager::~UIManager()
{
    // 所有子控件都会被它们的父控件自动删除，所以这里无需手动删除
}

void UIManager::setupUi(QMainWindow *mainWindow)
{
    // --- 1. 设置主窗口基本属性 ---
    mainWindow->setWindowTitle("音乐播放器");
    mainWindow->setMinimumSize(600, 400); // 设置最小尺寸

    // --- 顶部菜单栏 ---
    QMenuBar *menuBar = mainWindow->menuBar();

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu(tr("文件"));
    m_addFileAction = new QAction(tr("添加音频文件"), mainWindow);
    m_addFolderAction = new QAction(tr("从文件夹添加"), mainWindow);
    m_exitAction = new QAction(tr("退出"), mainWindow);
    fileMenu->addAction(m_addFileAction);
    fileMenu->addAction(m_addFolderAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    // 工具菜单
    QMenu *toolMenu = menuBar->addMenu(tr("工具"));
    m_extractAction = new QAction(tr("音频提取"), mainWindow);
    m_ttsAction = new QAction(tr("TTS"), mainWindow);
    m_sttAction = new QAction(tr("STT"), mainWindow);
    toolMenu->addAction(m_extractAction);
    toolMenu->addAction(m_ttsAction);
    toolMenu->addAction(m_sttAction);

    // 设置菜单
    m_settingsAction = new QAction(tr("设置"), mainWindow);
    menuBar->addAction(m_settingsAction); // 直接添加动作，而不是 QMenu

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu(tr("帮助"));
    m_aboutAction = new QAction(tr("关于..."), mainWindow);
    helpMenu->addAction(m_aboutAction);

    // 创建中心Widget
    QWidget *centralWidget = new QWidget(mainWindow);
    mainWindow->setCentralWidget(centralWidget);

    // --- 2. 创建主布局 ---
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15); // 内边距
    mainLayout->setSpacing(10); // 控件间距

    // --- 3. 顶部信息区 ---
    QHBoxLayout *infoLayout = new QHBoxLayout();
    infoLayout->setAlignment(Qt::AlignCenter);  // 整体居中对齐

    // 歌曲名
    m_currentSongTitleLabel = new MarqueeLabel(centralWidget);
    m_currentSongTitleLabel->setText("未选择歌曲");
    m_currentSongTitleLabel->setFont(QFont("Segoe UI", 14, QFont::Bold));
    m_currentSongTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_currentSongTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_currentSongTitleLabel->setStyleSheet(R"(
        color: #4CAF50;
        background-color: transparent;
    )");

    infoLayout->addWidget(m_currentSongTitleLabel, 2);

    // 播放模式
    m_playModeLabel = new QLabel("顺序播放", centralWidget);
    m_playModeLabel->setAlignment(Qt::AlignCenter);
    m_playModeLabel->setFixedWidth(100);
    infoLayout->addWidget(m_playModeLabel, 0);

    // 音频信息
    m_audioInfoLabel = new QLabel("- Hz | - | - kbps", centralWidget);
    m_audioInfoLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_audioInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    infoLayout->addWidget(m_audioInfoLabel, 2);

    mainLayout->addLayout(infoLayout);


    // --- 4. 播放列表区 ---
    m_playlistWidget = new QListWidget(centralWidget);
    m_playlistWidget->setObjectName("playlistWidget");
    mainLayout->addWidget(m_playlistWidget);
    m_playlistWidget->setStyleSheet("QListWidget::item { background: transparent; }");
    m_playlistWidget->setSelectionMode(QAbstractItemView::NoSelection);


    // --- 5. 播放进度条 + 时间标签 ---
    QHBoxLayout *progressLayout = new QHBoxLayout();

    m_currentTimeLabel = new QLabel("00:00", centralWidget);
    m_currentTimeLabel->setFixedWidth(50);
    m_currentTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_progressBar = new QSlider(Qt::Horizontal, centralWidget);
    m_progressBar->setRange(0, 1000);
    m_progressBar->setObjectName("progressBar");
    m_progressBar->setTracking(true);

    m_totalTimeLabel = new QLabel("00:00", centralWidget);
    m_totalTimeLabel->setFixedWidth(50);
    m_totalTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    progressLayout->addWidget(m_currentTimeLabel);
    progressLayout->addWidget(m_progressBar, 1); // 拉伸
    progressLayout->addWidget(m_totalTimeLabel);

    mainLayout->addLayout(progressLayout);

    // --- 6. 底部控制区 ---
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(15); // 控制按钮之间的间距
    controlsLayout->setAlignment(Qt::AlignCenter); // 按钮组居中

    // 添加文件按钮
    m_addFilesButton = createControlButton(":/icons/add.png", "addFilesButton");
    controlsLayout->addWidget(m_addFilesButton);

    // 上一曲按钮
    m_prevButton = createControlButton(":/icons/previous.png", "prevButton");
    controlsLayout->addWidget(m_prevButton);

    // 播放/暂停按钮 (初始设置为播放图标)
    m_playPauseButton = createControlButton(":/icons/play.png", "playPauseButton");
    setPlayPauseButtonIcon(false); // 初始为暂停状态，显示播放图标
    controlsLayout->addWidget(m_playPauseButton);

    // 下一曲按钮
    m_nextButton = createControlButton(":/icons/next.png", "nextButton");
    controlsLayout->addWidget(m_nextButton);

    // 单曲循环按钮 (初始为非激活状态)
    m_loopButton = createControlButton(":/icons/sequential.png", "loopButton");
    controlsLayout->addWidget(m_loopButton);

    // 音量控制
    m_volumeSlider = new QSlider(Qt::Horizontal, centralWidget);
    m_volumeSlider->setRange(0, 100); // 0% 到 100%
    m_volumeSlider->setValue(50); // 默认音量
    m_volumeSlider->setFixedWidth(100); // 固定宽度，使其紧凑
    m_volumeSlider->setObjectName("volumeSlider");
    controlsLayout->addWidget(m_volumeSlider);

    mainLayout->addLayout(controlsLayout); // 将控制区布局添加到主布局

    // --- 7. 应用样式表 ---
    applyStyleSheet(mainWindow);
}

// 辅助函数：创建控制按钮
QPushButton* UIManager::createControlButton(const QString &iconPath, const QString &objectName)
{
    QPushButton *button = new QPushButton(static_cast<QWidget*>(parent())); // parent() 实际上是 UIManager 传入的 QWidget *parent
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(32, 32)); // 图标大小
    button->setObjectName(objectName); // 用于 QSS
    button->setFixedSize(48, 48); // 固定按钮大小，保持一致性
    return button;
}

// 设置播放/暂停按钮图标
void UIManager::setPlayPauseButtonIcon(bool isPlaying)
{
    if (m_playPauseButton) {
        if (isPlaying) {
            m_playPauseButton->setIcon(QIcon(":/icons/pause.png"));
            m_playPauseButton->setObjectName("playPauseButton_playing"); // 可以在QSS中针对不同状态做更多样式
        } else {
            m_playPauseButton->setIcon(QIcon(":/icons/play.png"));
            m_playPauseButton->setObjectName("playPauseButton_paused");
        }
    }
}

// UIManager 中添加方法更新图标：
void UIManager::setLoopButtonIcon(const QString &iconPath)
{
    if (m_loopButton) {
        m_loopButton->setIcon(QIcon(iconPath));
    }
}

// 应用 QSS 样式表
void UIManager::applyStyleSheet(QMainWindow *mainWindow)
{
    QString styleSheet = R"(

        /* 播放列表 */
        QListWidget#playlistWidget {
            background-color: #FFFFFF; /* 白色列表背景，清晰简洁 */
            border: 1px solid #E0E0E0; /* 浅灰色边框 */
            border-radius: 8px; /* 列表圆角 */
            alternate-background-color: #F0F0F0; /* 交替行颜色 */
            padding: 5px;
            font-size: 15px; /* 稍微大一点 */
            outline: none; /* 移除焦点边框 */
        }
        /* 滚动条美化 (QListWidget内部) */
        QScrollBar:vertical {
            border: none;
            background: #E0E0E0; /* 浅灰色滚动条背景 */
            width: 8px;
            margin: 0px 0px 0px 0px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #BDBDBD; /* 柔和的灰色滑块 */
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }


        /* 按钮通用样式 */
        QPushButton {
            background-color: #E0E0E0; /* 按钮默认背景色，柔和的灰色 */
            border: none;
            color: #666666; /* 按钮图标/文字颜色，比背景稍深 */
            padding: 10px;
            border-radius: 28px; /* 圆形按钮 */
            outline: none;
        }

        QPushButton:hover {
            background-color: #D0D0D0; /* 鼠标悬停时背景更深一点 */
            color: #333333; /* 鼠标悬停时图标更深 */
        }

        QPushButton:pressed {
            background-color: #A0A0A0; /* 按下时更深的灰色 */
            color: #1A1A1A;
        }

        /* 播放/暂停按钮特定样式，使其更突出 */
        QPushButton#playPauseButton_playing, QPushButton#playPauseButton_paused {
            background-color: #4CAF50; /* 强调色背景 */
            color: white; /* 白色图标 */
            min-width: 60px;
            min-height: 60px;
            max-width: 60px;
            max-height: 60px;
            border-radius: 30px; /* 更大的圆形 */
        }

        QPushButton#playPauseButton_playing:hover, QPushButton#playPauseButton_paused:hover {
            background-color: #66BB6A; /* 悬停时更亮的强调色 */
        }

        QPushButton#playPauseButton_playing:pressed, QPushButton#playPauseButton_paused:pressed {
            background-color: #388E3C; /* 按下时更深的强调色 */
        }


        /* 进度条 */
        QSlider#progressBar::groove:horizontal {
            border: 1px solid #E0E0E0; /* 浅边框 */
            height: 8px; /* 轨道高度 */
            background: #F0F0F0; /* 柔和的背景 */
            margin: 2px 0;
            border-radius: 4px;
        }

        QSlider#progressBar::handle:horizontal {
            background: #4CAF50; /* 强调色滑块 */
            border: 1px solid #4CAF50;
            width: 18px; /* 滑块宽度 */
            height: 18px; /* 滑块高度 */
            margin: -6px 0; /* 垂直居中 */
            border-radius: 9px; /* 圆形滑块 */
        }

        QSlider#progressBar::add-page:horizontal {
            background: #D0D0D0; /* 已播放部分的轨道颜色 */
        }

        QSlider#progressBar::sub-page:horizontal {
            background: #66BB6A; /* 未播放部分的轨道颜色 (已填充部分) */
        }

        /* 音量条 */
        QSlider#volumeSlider::groove:horizontal {
            border: 1px solid #E0E0E0;
            height: 6px;
            background: #F0F0F0;
            margin: 2px 0;
            border-radius: 3px;
        }
        QSlider#volumeSlider::handle:horizontal {
            background: #4CAF50;
            border: 1px solid #4CAF50;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider#volumeSlider::add-page:horizontal {
            background: #D0D0D0;
        }
        QSlider#volumeSlider::sub-page:horizontal {
            background: #66BB6A;
        }

        QPushButton#settingsButton {
            background-color: #E0E0E0;
            color: #666666;
        }

        QPushButton#settingsButton:hover {
            background-color: #D0D0D0;
            color: #333333;
        }
    )";

    mainWindow->setStyleSheet(styleSheet);
}
