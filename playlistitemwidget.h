// playlistitemwidget.h
#ifndef PLAYLISTITEMWIDGET_H
#define PLAYLISTITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QString>
#include <QPainter>
#include <QStyleOption>

class PlaylistItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistItemWidget(const QString &fileName, QWidget *parent = nullptr);

    void setSelected(bool selected);
    QString fileName() const { return m_label->text(); }

signals:
    void requestDelete();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QLabel *m_label;
    QPushButton *m_deleteButton;
    bool m_selected;
};

#endif // PLAYLISTITEMWIDGET_H
