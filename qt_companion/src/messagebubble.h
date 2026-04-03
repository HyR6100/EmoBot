#pragma once

#include <QWidget>

class QLabel;

class MessageBubble : public QWidget
{
    Q_OBJECT
public:
    explicit MessageBubble(bool isUser, QWidget *parent = nullptr);

    void setText(const QString &text);

    void setBackgroundColor(const QColor &color);
    void setTextColor(const QColor &color);

    void setAvatarPixmap(const QPixmap &pixmap);

private:
    void ensureAvatar();

    QLabel *m_label = nullptr;
    QLabel *m_avatar = nullptr;
};

