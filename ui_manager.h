#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <QtWidgets>
#include <QIcon>
#include <QLabel>
#include "marqueelabel.h"

// 前向声明，避免交叉引用，因为 UIManager 会在 MainWindow 中使用
class MainWindow;

class UIManager : public QObject
{
    Q_OBJECT

public:
    explicit UIManager(QWidget *parent = nullptr);
    ~UIManager();

    // 初始化并构建所有UI元素
    void setupUi(QMainWindow *mainWindow);

    // 获取UI元素的公共接口
    MarqueeLabel *currentSongTitleLabel() const { return m_currentSongTitleLabel; }
    QLabel *playModeLabel() const { return m_playModeLabel; }
    QLabel *audioInfoLabel() const { return m_audioInfoLabel; }

    QListWidget *playlistWidget() const { return m_playlistWidget; }
    QPushButton *addFilesButton() const { return m_addFilesButton; }
    QPushButton *prevButton() const { return m_prevButton; }
    QPushButton *playPauseButton() const { return m_playPauseButton; }
    QPushButton *nextButton() const { return m_nextButton; }
    QPushButton *loopButton() const { return m_loopButton; }
    QSlider *progressBar() const { return m_progressBar; }
    QSlider *volumeSlider() const { return m_volumeSlider; }

    // 更新播放/暂停按钮图标的方法
    void setPlayPauseButtonIcon(bool isPlaying);
    // 更新图标
    void setLoopButtonIcon(const QString &iconPath);

    QLabel *currentTimeLabel() const { return m_currentTimeLabel; }
    QLabel *totalTimeLabel() const { return m_totalTimeLabel; }

    QPushButton *deleteButton() const { return m_deleteButton; }

    // menu bar
    // file
    QAction *addFileAction() const { return m_addFileAction; }
    QAction *addFolderAction() const { return m_addFolderAction; }
    QAction *exitAction() const { return m_exitAction; }
    // tool
    QAction *extraAction() const { return m_extractAction; }
    QAction *ttsAction() const { return m_ttsAction; }
    QAction *sttAction() const { return m_sttAction; }
    // setting
    QAction *settingsAction() const { return m_settingsAction; }
    // help
    QAction *aboutAction() const { return m_aboutAction; }


private:
    // menu bar
    // 文件菜单 Actions
    QAction *m_addFileAction;
    QAction *m_addFolderAction;
    QAction *m_exitAction;

    // 工具菜单 Actions
    QAction *m_extractAction;
    QAction *m_ttsAction;
    QAction *m_sttAction;

    // 设置
    QAction *m_settingsAction;

    // 帮助菜单
    QAction *m_aboutAction;
    // --- UI 元素私有成员变量 ---
    MarqueeLabel *m_currentSongTitleLabel;
    QLabel *m_playModeLabel;
    QLabel *m_audioInfoLabel;

    QListWidget *m_playlistWidget;

    // 底部控制区按钮
    QPushButton *m_addFilesButton;
    QPushButton *m_prevButton;
    QPushButton *m_playPauseButton;
    QPushButton *m_nextButton;
    QPushButton *m_loopButton;

    QSlider *m_progressBar;
    QSlider *m_volumeSlider;

    QLabel *m_currentTimeLabel;
    QLabel *m_totalTimeLabel;

    QPushButton *m_deleteButton;

    QPushButton *m_settingsButton;

    // 辅助函数，用于设置按钮样式和图标
    QPushButton* createControlButton(const QString &iconPath, const QString &objectName);
    // 应用 QSS 样式
    void applyStyleSheet(QMainWindow *mainWindow);
};

#endif // UI_MANAGER_H
