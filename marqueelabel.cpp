#include "marqueelabel.h"
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>

MarqueeLabel::MarqueeLabel(QWidget *parent)
    : QLabel(parent),
      m_offset(0),
      m_step(2),
      m_textWidth(0),
      m_hovering(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(28); // 建议高度
    setText("");
    connect(&m_timer, &QTimer::timeout, this, &MarqueeLabel::updateOffset);
    m_timer.setInterval(30); // 每 30ms 移动一次
}

void MarqueeLabel::setText(const QString &text)
{
    m_fullText = text;
    QFontMetrics fm(font());
    m_textWidth = fm.horizontalAdvance(m_fullText);
    m_offset = 0;

    if (m_textWidth > width()) {
        m_timer.start();
    } else {
        m_timer.stop();
    }

    update();
}

void MarqueeLabel::setSpeed(int pixelsPerStep)
{
    m_step = pixelsPerStep;
}

void MarqueeLabel::updateOffset()
{
    if (m_textWidth <= width()) {
        m_timer.stop();
        return;
    }

    m_offset += m_step;
    if (m_offset > m_textWidth + 50) {
        m_offset = 0;
    }

    update();
}

void MarqueeLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.setFont(font());

    int y = (height() + fontMetrics().ascent() - fontMetrics().descent()) / 2;

    if (m_textWidth <= width()) {
        painter.drawText(0, y, m_fullText);
        return;
    }

    int x = -m_offset;
    painter.drawText(x, y, m_fullText);
    painter.drawText(x + m_textWidth + 50, y, m_fullText); // 第二段，留出空隙
}

void MarqueeLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    QFontMetrics fm(font());
    m_textWidth = fm.horizontalAdvance(m_fullText);
    m_offset = 0;

    if (m_textWidth > width() && !m_hovering) {
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void MarqueeLabel::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovering = true;
    m_timer.stop(); // 悬停暂停
}

void MarqueeLabel::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovering = false;
    if (m_textWidth > width()) {
        m_timer.start();
    }
}
