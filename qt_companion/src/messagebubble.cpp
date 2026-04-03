#include "messagebubble.h"

#include <QColor>
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

MessageBubble::MessageBubble(bool isUser, QWidget *parent)
    : QWidget(parent)
{
    m_avatar = new QLabel(this);
    m_avatar->setFixedSize(40, 40);
    m_avatar->setAlignment(Qt::AlignCenter);
    // Qt Widgets stylesheet doesn't support CSS 'overflow'; we already mask the pixmap to a circle.
    m_avatar->setStyleSheet("border-radius:20px; border:1px solid rgba(255,255,255,0.75); background:rgba(255,255,255,0.35);");

    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);
    if (isUser) {
        // User: avatar on the right
        layout->addWidget(m_label, 0, Qt::AlignLeft);
        layout->addWidget(m_avatar, 0, Qt::AlignVCenter);
    } else {
        // Assistant: avatar on the left
        layout->addWidget(m_avatar, 0, Qt::AlignVCenter);
        layout->addWidget(m_label, 0, Qt::AlignLeft);
    }

    // Default theme colors; can be overridden.
    if (isUser) {
        setBackgroundColor(QColor("#ffd1ff"));
        setTextColor(QColor("#2b0038"));
    } else {
        setBackgroundColor(QColor("#e7f2ff"));
        setTextColor(QColor("#2b0038"));
    }

    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    setStyleSheet("border-radius:16px;");
}

void MessageBubble::setText(const QString &text)
{
    m_label->setText(text);
}

void MessageBubble::setBackgroundColor(const QColor &color)
{
    setStyleSheet(QString("border-radius:16px; background:%1; border:1px solid rgba(255,255,255,0.45);").arg(color.name()));
}

void MessageBubble::setTextColor(const QColor &color)
{
    m_label->setStyleSheet(QString("color:%1;").arg(color.name()));
}

void MessageBubble::setAvatarPixmap(const QPixmap &pixmap)
{
    ensureAvatar();
    if (!pixmap.isNull()) {
        // Paint into a circle for a clean avatar.
        QPixmap out(m_avatar->size());
        out.fill(Qt::transparent);

        QPainter p(&out);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addEllipse(QRectF(0, 0, out.width(), out.height()));
        p.setClipPath(path);
        p.drawPixmap(out.rect(), pixmap);
        p.end();

        m_avatar->setPixmap(out);
    }
}

void MessageBubble::ensureAvatar()
{
    if (!m_avatar) return;
    const QPixmap pm = m_avatar->pixmap(Qt::ReturnByValue);
    if (pm.isNull()) {
        // fallback: draw a tiny placeholder gradient.
        QPixmap ph(m_avatar->size());
        ph.fill(Qt::transparent);
        QPainter p(&ph);
        p.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient g(0, 0, ph.width(), ph.height());
        g.setColorAt(0, QColor("#ff6fd6"));
        g.setColorAt(1, QColor("#7cb8ff"));
        p.setBrush(g);
        p.setPen(Qt::NoPen);
        p.drawEllipse(ph.rect());
        p.end();
        m_avatar->setPixmap(ph);
    }
}

