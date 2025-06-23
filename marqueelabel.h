#pragma once

#include <QLabel>
#include <QTimer>

class MarqueeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit MarqueeLabel(QWidget *parent = nullptr);
    void setText(const QString &text);   // 设置文字
    void setSpeed(int pixelsPerStep);    // 设置滚动速度

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void updateOffset(); // 滚动偏移更新

private:
    QString m_fullText;
    int m_offset;
    int m_step;
    int m_textWidth;
    QTimer m_timer;
    bool m_hovering;
};
