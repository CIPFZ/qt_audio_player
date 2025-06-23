#include "playlistitemwidget.h"

PlaylistItemWidget::PlaylistItemWidget(const QString &fileName, QWidget *parent)
    : QWidget(parent), m_selected(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    m_label = new QLabel(fileName, this);
    m_label->setStyleSheet("font-size: 15px; color: #333333;");
    layout->addWidget(m_label);

    layout->addStretch();

    m_deleteButton = new QPushButton(this);
    m_deleteButton->setIcon(QIcon(":/icons/delete.png"));
    m_deleteButton->setIconSize(QSize(16, 16));
    m_deleteButton->setFixedSize(24, 24);
    m_deleteButton->setFlat(true); // 不要边框
    m_deleteButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background-color: transparent;
        }
        QPushButton:hover {
            background-color: #ffeeee;
            border-radius: 12px;
        }
    )");
    layout->addWidget(m_deleteButton);

    connect(m_deleteButton, &QPushButton::clicked, this, &PlaylistItemWidget::requestDelete);

}

void PlaylistItemWidget::setSelected(bool selected)
{
    m_selected = selected;
    update();
}

void PlaylistItemWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);

    QColor bgColor;
    if (m_selected) {
        bgColor = QColor("#E8F5E9");
    } else if (underMouse()) {
        bgColor = QColor("#F5F5F5"); // 悬浮背景
    } else {
        bgColor = Qt::transparent;
    }

    p.fillRect(rect(), bgColor);
    QWidget::paintEvent(event); // 继续绘制子控件
}

void PlaylistItemWidget::enterEvent(QEvent *event)
{
    update(); // 触发 paintEvent 更新背景
    QWidget::enterEvent(event);
}

void PlaylistItemWidget::leaveEvent(QEvent *event)
{
    update(); // 鼠标离开时刷新背景
    QWidget::leaveEvent(event);
}



