#include "mainwindow.h"

#include "chatinputedit.h"
#include "messagebubble.h"
#include "personadialog.h"

#include "companionbackend.h"
#include "mjpegviewerwidget.h"

#include <QColor>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QLayoutItem>
#include <QVBoxLayout>
#include <QTextCursor>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QApplication>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QSizePolicy>

static QColor bubbleTextColor(const QColor &bg)
{
    // For our light theme bubbles, use a dark purple.
    Q_UNUSED(bg);
    return QColor("#2b0038");
}

static QPixmap makeAvatarPlaceholder(const QString &centerText, const QColor &c1, const QColor &c2)
{
    const int w = 34;
    const int h = 34;
    QPixmap out(w, h);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient g(0, 0, w, h);
    g.setColorAt(0, c1);
    g.setColorAt(1, c2);
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, w, h);

    QFont f = p.font();
    f.setBold(true);
    f.setPointSize(12);
    p.setFont(f);
    p.setPen(QColor("#ffffff"));

    QRect textRect(0, 0, w, h);
    p.drawText(textRect, Qt::AlignCenter, centerText);
    p.end();
    return out;
}

QString MainWindow::emotionTail(const QString &emotion)
{
    if (emotion == "happy") return QStringLiteral(" (^_^)");
    if (emotion == "sad") return QStringLiteral(" (T_T)");
    if (emotion == "clingy") return QStringLiteral(" (hug~)");
    if (emotion == "excited") return QStringLiteral(" (!!!)");
    return QString();
}

QColor MainWindow::assistantBubbleColor(const QString &emotion)
{
    Q_UNUSED(emotion);
    // Always keep 夏以昼 bubble blue (per your request).
    return QColor("#CDEBFF");
}

MainWindow::MainWindow(CompanionBackend *backend, QWidget *parent)
    : QMainWindow(parent)
    , m_backend(backend)
{
    setWindowTitle(QStringLiteral("EmoBot 梦幻陪伴系统"));
    resize(1120, 740);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // Light pink / light blue dreamy theme.
    setStyleSheet(R"(
        QMainWindow { background: #F6F0FF; }
        QWidget { font-family: Sans; }
        QLabel { color: #2b0038; }
        QPushButton {
            background-color: rgba(255, 111, 214, 0.20);
            border: 1px solid rgba(255, 102, 255, 0.75);
            border-radius: 14px;
            padding: 10px 14px;
        }
        QPushButton:hover {
            background-color: rgba(255, 111, 214, 0.28);
        }
        QGroupBox {
            border: 1px solid rgba(0, 150, 255, 0.25);
            border-radius: 16px;
            margin-top: 10px;
            background: rgba(255,255,255,0.55);
        }
        QScrollArea {
            border: 1px solid rgba(0, 150, 255, 0.25);
            border-radius: 16px;
            background: rgba(255,255,255,0.55);
        }
        ChatInputEdit, QTextEdit {
            border: 1px solid rgba(255, 102, 255, 0.35);
            border-radius: 16px;
            background: rgba(255,255,255,0.75);
        }
    )");

    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(12);

    // Left: conversation (expand), Right: video placeholder.
    QSplitter *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setHandleWidth(8);
    rootLayout->addWidget(splitter, 1);

    QWidget *leftPane = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    QWidget *statusRow = new QWidget(leftPane);
    QHBoxLayout *statusLayout = new QHBoxLayout(statusRow);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    m_statusLabel = new QLabel(QStringLiteral("就绪"), statusRow);
    statusLayout->addWidget(m_statusLabel, 1);

    m_resetBtn = new QPushButton(QStringLiteral("新对话"), statusRow);
    m_resetBtn->setMinimumWidth(150);
    statusLayout->addWidget(m_resetBtn, 0);
    leftLayout->addWidget(statusRow);

    // New message banner (WeChat-like floating chip): hidden by default.
    m_newMsgBanner = new QPushButton();
    m_newMsgBanner->setVisible(false);
    m_newMsgBanner->setFlat(true);
    m_newMsgBanner->setCursor(Qt::PointingHandCursor);
    m_newMsgBanner->setStyleSheet(R"(
        QPushButton {
            background: rgba(80, 160, 255, 0.86);
            border: 1px solid rgba(255, 255, 255, 0.75);
            color: #2b0038;
            border-radius: 16px;
            padding: 8px 12px;
            text-align: center;
            font-weight: 700;
        }
        QPushButton:hover {
            background: rgba(80, 160, 255, 0.95);
        }
    )");

    m_scroll = new QScrollArea(leftPane);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    leftLayout->addWidget(m_scroll, 1);
    m_newMsgBanner->setParent(m_scroll->viewport());
    m_newMsgBanner->raise();

    m_messagesContainer = new QWidget(m_scroll);
    m_messagesLayout = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setContentsMargins(14, 14, 14, 14);
    m_messagesLayout->setSpacing(10);
    m_messagesLayout->addStretch(1);
    m_scroll->setWidget(m_messagesContainer);

    // Track scroll position to show/hide new message banner.
    if (m_scroll && m_scroll->verticalScrollBar()) {
        connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
            if (isScrolledToBottom()) {
                m_unreadCount = 0;
                updateUnreadUi();
                // Keep pinned to the latest message when user is already at bottom.
                scrollToBottom();
            }
        });
    }

    connect(m_newMsgBanner, &QPushButton::clicked, this, [this]() {
        scrollToBottom();
        m_unreadCount = 0;
        updateUnreadUi();
    });

    QWidget *inputRow = new QWidget(leftPane);
    QHBoxLayout *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);

    m_inputEdit = new ChatInputEdit(inputRow);
    m_inputEdit->setFixedHeight(92);
    m_inputEdit->setPlaceholderText(QStringLiteral("输入你的话...（回车发送，Shift+回车换行）"));
    inputLayout->addWidget(m_inputEdit, 1);

    m_sendBtn = new QPushButton(QStringLiteral("发送"), inputRow);
    m_sendBtn->setFixedHeight(92);
    inputLayout->addWidget(m_sendBtn, 0);

    leftLayout->addWidget(inputRow, 0);
    splitter->addWidget(leftPane);

    QWidget *rightPane = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    QWidget *videoTopRow = new QWidget(rightPane);
    QHBoxLayout *videoTopLayout = new QHBoxLayout(videoTopRow);
    videoTopLayout->setContentsMargins(0, 0, 0, 0);

    m_personaBtn = new QPushButton(QStringLiteral("人物"), videoTopRow);
    m_personaBtn->setMinimumWidth(150);
    videoTopLayout->addWidget(m_personaBtn, 0);
    videoTopLayout->addStretch(1);

    // Audio controls.
    m_autoPlayCheck = new QCheckBox(QStringLiteral("自动播放"), videoTopRow);
    m_autoPlayCheck->setChecked(true);
    m_autoPlayCheck->setToolTip(QStringLiteral("收到夏以昼回复后，是否自动播放语音"));
    m_autoPlayCheck->setStyleSheet("QCheckBox { color:#6b3d86; font-weight:600; }");
    videoTopLayout->addWidget(m_autoPlayCheck, 0);

    m_playAudioBtn = new QPushButton(QStringLiteral("播放语音"), videoTopRow);
    m_playAudioBtn->setMinimumWidth(120);
    videoTopLayout->addWidget(m_playAudioBtn, 0);

    rightLayout->addWidget(videoTopRow, 0);

    m_videoPlaceholder = new QGroupBox(QStringLiteral("视频流（占位）"), rightPane);
    QVBoxLayout *videoLayout = new QVBoxLayout(m_videoPlaceholder);

    m_mjpegView = new MjpegViewerWidget(m_videoPlaceholder);
    m_mjpegView->setStyleSheet("background: rgba(255,255,255,0.15); border-radius:14px;");
    videoLayout->addWidget(m_mjpegView, 1);

    m_videoStatusLabel = new QLabel(QStringLiteral(
        "视频：① HTTP — 默认连接 http://127.0.0.1:8090/stream（可先运行 mjpeg_blender_server.py）\n"
        "② UDP — Blender 向 127.0.0.1:5005 发送 JPEG 字节（可拆多个 UDP 包，程序会按 JPEG 标记拼帧）"),
                                      m_videoPlaceholder);
    m_videoStatusLabel->setAlignment(Qt::AlignCenter);
    m_videoStatusLabel->setStyleSheet("color:#6b3d86;");
    videoLayout->addWidget(m_videoStatusLabel, 0);

    m_videoStatusDefaultText = m_videoStatusLabel->text();

    // Default MJPEG endpoint (you can change server port in mjpeg_blender_server.py).
    m_mjpegView->start(QStringLiteral("http://127.0.0.1:8090/stream"));

    rightLayout->addWidget(m_videoPlaceholder, 1);
    splitter->addWidget(rightPane);

    // Make the conversation area wider; keep video panel smaller.
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    m_videoPlaceholder->setMinimumWidth(320);
    m_videoPlaceholder->setMaximumWidth(420);

    // Persona dialog starts hidden.
    m_currentPersonaText = m_backend ? m_backend->defaultPersonaText() : QString();

    // Make the top buttons look nicer (light theme, rounded, flatter).
    const QString topBtnStyle = R"(
        QPushButton {
            background-color: rgba(80, 160, 255, 0.14);
            border: 1px solid rgba(0, 150, 255, 0.22);
            border-radius: 16px;
            padding: 6px 16px;
        }
        QPushButton:hover {
            background-color: rgba(80, 160, 255, 0.22);
        }
        QPushButton:pressed {
            background-color: rgba(255, 111, 214, 0.18);
        }
    )";
    setStyleSheet(styleSheet() + topBtnStyle);
    if (m_resetBtn) {
        m_resetBtn->setMinimumHeight(34);
        m_resetBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_resetBtn->setFlat(true);
        m_resetBtn->setCursor(Qt::PointingHandCursor);
    }
    if (m_personaBtn) {
        m_personaBtn->setMinimumHeight(34);
        m_personaBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_personaBtn->setFlat(true);
        m_personaBtn->setCursor(Qt::PointingHandCursor);
    }

    if (m_backend && m_mjpegView) {
        // Blender UDP JPEG → Qt（此前未连接 frameReady，导致界面一直没有画面）
        connect(m_backend, &CompanionBackend::frameReady, m_mjpegView, &MjpegViewerWidget::showFrame);
        connect(m_backend, &CompanionBackend::frameReady, this, [this](const QPixmap &) {
            if (m_videoStatusLabel)
                m_videoStatusLabel->setText(QStringLiteral("已接收 Blender UDP 视频帧（127.0.0.1:5005）"));
        });
    }

    if (m_backend) {
        connect(m_backend, &CompanionBackend::assistantReplyReady,
                this, &MainWindow::onAssistantReplyReady);
        connect(m_backend, &CompanionBackend::statusChanged,
                this, &MainWindow::onStatusChanged);
        connect(m_backend, &CompanionBackend::errorOccurred,
                this, &MainWindow::onErrorOccurred);
    }

    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        // Clear bubbles + reset backend conversation.
        // IMPORTANT: do not manually delete item->layout() here; QLayoutItem ownership can double-free and crash.
        if (!m_messagesLayout) return;
        while (QLayoutItem *item = m_messagesLayout->takeAt(0)) {
            if (QWidget *w = item->widget()) {
                w->deleteLater();
            }
            if (QLayout *childLayout = item->layout()) {
                // Recursively clear, but do not delete the layout directly.
                while (QLayoutItem *sub = childLayout->takeAt(0)) {
                    if (QWidget *sw = sub->widget()) sw->deleteLater();
                    if (QLayout *sl = sub->layout()) {
                        while (QLayoutItem *sub2 = sl->takeAt(0)) {
                            if (QWidget *sw2 = sub2->widget()) sw2->deleteLater();
                            delete sub2;
                        }
                    }
                    delete sub;
                }
            }
            delete item;
        }
        // Re-add stretch at the end.
        m_messagesLayout->addStretch(1);
        m_statusLabel->setText(QStringLiteral("已重置对话"));
        if (m_backend) m_backend->resetConversation();
        if (m_inputEdit) m_inputEdit->clear();
        m_currentPersonaText = m_backend ? m_backend->defaultPersonaText() : QString();
        setBusy(false);
        m_unreadCount = 0;
        updateUnreadUi();
    });

    connect(m_personaBtn, &QPushButton::clicked, this, &MainWindow::onPersonaClicked);

    connect(m_inputEdit, &ChatInputEdit::sendRequested, this, &MainWindow::onInputSendRequested);

    connect(m_playAudioBtn, &QPushButton::clicked, this, &MainWindow::onPlayAudioClicked);
}

MainWindow::~MainWindow() = default;

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    if (m_sendBtn) m_sendBtn->setEnabled(!busy);
    if (m_inputEdit) {
        // Keep focus by avoiding disabling the widget (disabled widgets often lose keyboard focus).
        m_inputEdit->setReadOnly(busy);
        m_inputEdit->setEnabled(true);
        if (!busy) {
            m_inputEdit->setFocus();
            m_inputEdit->moveCursor(QTextCursor::End);
        }
    }
}

void MainWindow::appendMessage(bool isUser, const QString &text, const QString &emotionForAssistant)
{
    if (!m_messagesLayout) return;

    const int distBefore = distanceToBottom();
    const bool nearBottomBefore = (distBefore <= 220);

    MessageBubble *bubble = new MessageBubble(isUser);

    // Avatar: load from embedded resources (fallback to placeholder).
    if (isUser) {
        const QPixmap px(QStringLiteral(":/resources/avatars/user.png"));
        bubble->setAvatarPixmap(px.isNull()
                                    ? makeAvatarPlaceholder(QStringLiteral("你"), QColor("#FF6FD6"), QColor("#7cb8ff"))
                                    : px);
    } else {
        const QPixmap px(QStringLiteral(":/resources/avatars/xia.png"));
        bubble->setAvatarPixmap(px.isNull()
                                    ? makeAvatarPlaceholder(QStringLiteral("夏"), QColor("#7cb8ff"), QColor("#CDEBFF"))
                                    : px);
    }

    if (isUser) {
        bubble->setTextColor(QColor("#2b0038"));
        bubble->setBackgroundColor(QColor("#FFD1FF"));
        bubble->setText(text);
    } else {
        const QColor bg = assistantBubbleColor(emotionForAssistant);
        bubble->setBackgroundColor(bg);
        bubble->setTextColor(bubbleTextColor(bg));
        bubble->setText(text);
    }

    QHBoxLayout *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    if (isUser) {
        row->addStretch(1);
        row->addWidget(bubble, 0, Qt::AlignRight);
    } else {
        row->addWidget(bubble, 0, Qt::AlignLeft);
        row->addStretch(1);
    }

    // Insert before the stretch item at the end.
    const int count = m_messagesLayout->count();
    if (count > 0) {
        m_messagesLayout->insertLayout(count - 1, row);
    } else {
        m_messagesLayout->addLayout(row);
    }

    // Auto-scroll rules:
    // - User messages: always scroll to bottom.
    // - Assistant messages: only scroll if user is already at bottom; otherwise show banner + unread count.
    if (isUser || m_forceFollowAssistantOnce || nearBottomBefore) {
        scrollToBottom();
        if (!isUser && m_forceFollowAssistantOnce) {
            m_forceFollowAssistantOnce = false;
        }
    } else {
        m_unreadCount++;
        updateUnreadUi();
        // WeChat-like gentle alert (taskbar/dock flash).
        QApplication::alert(this, 0);
    }
}

bool MainWindow::isScrolledToBottom() const
{
    if (!m_scroll || !m_scroll->verticalScrollBar())
        return true;
    QScrollBar *bar = m_scroll->verticalScrollBar();
    // Treat "almost at bottom" as latest to avoid false unread alerts.
    return (bar->maximum() - bar->value()) <= 28;
}

int MainWindow::distanceToBottom() const
{
    if (!m_scroll || !m_scroll->verticalScrollBar())
        return 0;
    QScrollBar *bar = m_scroll->verticalScrollBar();
    return qMax(0, bar->maximum() - bar->value());
}

void MainWindow::scrollToBottom()
{
    if (!m_scroll || !m_scroll->verticalScrollBar())
        return;
    QScrollBar *bar = m_scroll->verticalScrollBar();
    // Smooth scroll to latest (wechat-like feel).
    if (!m_scrollAnim) {
        m_scrollAnim = new QPropertyAnimation(bar, "value", this);
        m_scrollAnim->setDuration(180);
        m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    }
    m_scrollAnim->stop();
    m_scrollAnim->setStartValue(bar->value());
    m_scrollAnim->setEndValue(bar->maximum());
    m_scrollAnim->start();
}

static void forceScrollBottomNow(QScrollArea *scroll)
{
    if (!scroll || !scroll->verticalScrollBar())
        return;
    QScrollBar *bar = scroll->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::updateUnreadUi()
{
    if (!m_newMsgBanner)
        return;
    if (m_unreadCount <= 0) {
        m_newMsgBanner->setVisible(false);
        m_newMsgBanner->setText(QString());
        return;
    }
    const QString title = (m_unreadCount == 1)
                              ? QStringLiteral("1 条新消息")
                              : QStringLiteral("%1 条新消息").arg(m_unreadCount);
    m_newMsgBanner->setText(title);

    // Floating at bottom-right of chat viewport.
    if (m_scroll && m_scroll->viewport()) {
        QWidget *vp = m_scroll->viewport();
        const int w = 150;
        const int h = 34;
        const int margin = 14;
        m_newMsgBanner->setFixedSize(w, h);
        m_newMsgBanner->move(vp->width() - w - margin, vp->height() - h - margin);
    }
    m_newMsgBanner->setVisible(true);
    m_newMsgBanner->raise();
}

void MainWindow::onSendClicked()
{
    if (!m_inputEdit) return;
    const QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    onInputSendRequested(text);
}

void MainWindow::onInputSendRequested(const QString &text)
{
    if (!m_backend) return;
    if (m_busy) return;

    const QString userText = text.trimmed();
    if (userText.isEmpty()) return;

    appendMessage(true, userText);
    if (m_inputEdit) {
        m_inputEdit->clear();
        m_inputEdit->setFocus();
        m_inputEdit->moveCursor(QTextCursor::End);
    }

    m_statusLabel->setText(QStringLiteral("思考中..."));
    setBusy(true);
    // The next assistant reply should follow to latest by default.
    m_forceFollowAssistantOnce = true;

    m_backend->sendMessage(userText, m_currentPersonaText);
}

void MainWindow::onAssistantReplyReady(const QString &reply, const QString &emotion, const QString &gesture)
{
    Q_UNUSED(gesture);
    const bool shouldForceFollow = m_forceFollowAssistantOnce || isScrolledToBottom();
    const QString showText = reply + emotionTail(emotion);
    appendMessage(false, showText, emotion);
    if (m_forceFollowAssistantOnce) {
        m_forceFollowAssistantOnce = false;
    }

    // Robust follow-to-latest:
    // don't rely on a fixed delay (like 80ms), because heavy model response/UI lag can exceed it.
    if (shouldForceFollow) {
        forceScrollBottomNow(m_scroll);
        QTimer::singleShot(0, this, [this]() {
            forceScrollBottomNow(m_scroll);
            scrollToBottom();
            m_unreadCount = 0;
            updateUnreadUi();
        });

        // Retry hard-snapping for up to ~3 seconds (30 * 100ms), to survive long layout/network/model stalls.
        if (!m_followLatestRetryTimer) {
            m_followLatestRetryTimer = new QTimer(this);
            m_followLatestRetryTimer->setInterval(100);
            connect(m_followLatestRetryTimer, &QTimer::timeout, this, [this]() {
                if (m_followLatestRetryLeft <= 0) {
                    m_followLatestRetryTimer->stop();
                    return;
                }
                m_followLatestRetryLeft--;
                forceScrollBottomNow(m_scroll);
                if (m_unreadCount > 0) {
                    m_unreadCount = 0;
                    updateUnreadUi();
                }
                if (isScrolledToBottom()) {
                    // already stable at latest; can stop earlier
                    m_followLatestRetryLeft = 0;
                    m_followLatestRetryTimer->stop();
                }
            });
        }
        m_followLatestRetryLeft = 30;
        m_followLatestRetryTimer->start();
    }

    m_statusLabel->setText(QStringLiteral("已回复"));
    setBusy(false);

    m_lastAssistantText = reply;
    maybeAutoPlayTTS(reply);
}

void MainWindow::onStatusChanged(const QString &status)
{
    if (!m_busy && m_statusLabel) {
        m_statusLabel->setText(status);
    }
}

void MainWindow::onErrorOccurred(const QString &message)
{
    if (m_statusLabel) m_statusLabel->setText(message);
    setBusy(false);
    QMessageBox::warning(this, QStringLiteral("错误"), message);
}

void MainWindow::onPersonaClicked()
{
    if (!m_backend) return;

    if (!m_personaDialog) {
        m_personaDialog = new PersonaDialog(m_backend->defaultPersonaText(), this);
    }
    // Re-create each time to pick up base changes and keep UI consistent.
    delete m_personaDialog;
    m_personaDialog = new PersonaDialog(m_backend->defaultPersonaText(), this);

    if (m_personaDialog->exec() == QDialog::Accepted) {
        m_currentPersonaText = m_personaDialog->personaText();
        m_statusLabel->setText(QStringLiteral("人物风格已更新"));
    }
}

void MainWindow::onPlayAudioClicked()
{
    if (m_lastAssistantText.trimmed().isEmpty())
        return;
    startTTS(m_lastAssistantText);
}

void MainWindow::maybeAutoPlayTTS(const QString &text)
{
    m_lastAssistantText = text;
    if (!m_autoPlayCheck)
        return;
    if (!m_autoPlayCheck->isChecked())
        return;

    // Auto-play
    startTTS(text);
}

void MainWindow::stopTTS()
{
    m_ttsBusy = false;
    if (m_pulseTimer) {
        m_pulseTimer->stop();
        delete m_pulseTimer;
        m_pulseTimer = nullptr;
    }
    if (m_ttsProc) {
        m_ttsProc->kill();
        m_ttsProc->deleteLater();
        m_ttsProc = nullptr;
    }
    if (m_ffplayProc) {
        m_ffplayProc->kill();
        m_ffplayProc->deleteLater();
        m_ffplayProc = nullptr;
    }

    if (m_videoPlaceholder) {
        m_videoPlaceholder->setStyleSheet(
            "QGroupBox { border: 1px solid rgba(0,150,255,0.25); border-radius:16px; background: rgba(255,255,255,0.55); margin-top:10px; }");
    }

    if (m_videoStatusLabel) {
        m_videoStatusLabel->setText(m_videoStatusDefaultText);
    }
}

void MainWindow::pulseSpeaking(bool speaking)
{
    if (!m_videoPlaceholder)
        return;

    if (!speaking) {
        stopTTS();
        return;
    }

    if (!m_pulseTimer) {
        m_pulseTimer = new QTimer(this);
        connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
            m_pulsePhase = (m_pulsePhase + 1) % 2;
            const QString border = (m_pulsePhase == 0)
                                        ? QStringLiteral("rgba(0,150,255,0.65)")
                                        : QStringLiteral("rgba(255,111,214,0.65)");
            if (m_videoPlaceholder) {
                m_videoPlaceholder->setStyleSheet(
                    QString("QGroupBox { border: 2px solid %1; border-radius:16px; background: rgba(255,255,255,0.55); margin-top:10px; }")
                        .arg(border));
            }
            if (m_videoStatusLabel) {
                // Small hint animation
                const QString base = m_videoStatusDefaultText;
                m_videoStatusLabel->setText(base + ((m_pulsePhase == 0) ? QStringLiteral("  ♪") : QStringLiteral("")));
            }
        });
    }

    if (m_pulseTimer && !m_pulseTimer->isActive())
        m_pulseTimer->start(160);
}

void MainWindow::startTTS(const QString &text)
{
    if (text.trimmed().isEmpty())
        return;
    // UI widgets are always created in our constructor; this is just defensive.
    if (!m_autoPlayCheck && !m_playAudioBtn) {
        // continue anyway
    }

    // Ensure only one playback at a time.
    stopTTS();

    m_ttsBusy = true;
    m_lastAssistantText = text;

    if (m_videoStatusLabel) {
        m_videoStatusLabel->setText(QStringLiteral("正在生成语音..."));
    }

    // Prepare output path.
    const QString outPath =
        QDir::tempPath() + QLatin1String("/emobot_tts_") +
        QString::number(QDateTime::currentMSecsSinceEpoch()) + QLatin1String(".mp3");
    m_lastAudioPath = outPath;

    const QString scriptPath = QDir(QCoreApplication::applicationDirPath()).filePath("tts_edge.py");
    const QString pythonPath = QLatin1String("/home/suyue/miniconda3/envs/isaacsim/bin/python");

    m_ttsProc = new QProcess(this);
    m_ttsProc->setProgram(pythonPath);
    m_ttsProc->setArguments(QStringList()
                             << scriptPath
                             << QStringLiteral("--text") << text
                             << QStringLiteral("--out") << outPath
                             << QStringLiteral("--voice") << QStringLiteral("zh-CN-YunxiNeural"));

    // During generation: pulse gently.
    pulseSpeaking(true);

    connect(m_ttsProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outPath](int exitCode, QProcess::ExitStatus st) {
                Q_UNUSED(st);
                if (exitCode != 0) {
                    if (m_videoStatusLabel)
                        m_videoStatusLabel->setText(QStringLiteral("语音生成失败"));
                    stopTTS();
                    return;
                }

                // Start ffplay.
                if (!QFile::exists(outPath)) {
                    if (m_videoStatusLabel)
                        m_videoStatusLabel->setText(QStringLiteral("语音文件不存在"));
                    stopTTS();
                    return;
                }

                if (m_videoStatusLabel)
                    m_videoStatusLabel->setText(QStringLiteral("语音播放中..."));

                if (m_ffplayProc) {
                    m_ffplayProc->kill();
                    m_ffplayProc->deleteLater();
                    m_ffplayProc = nullptr;
                }

                m_ffplayProc = new QProcess(this);
                m_ffplayProc->setProgram(QStringLiteral("ffplay"));
                m_ffplayProc->setArguments(QStringList()
                                            << QStringLiteral("-nodisp")
                                            << QStringLiteral("-autoexit")
                                            << QStringLiteral("-loglevel") << QStringLiteral("quiet")
                                            << outPath);

                connect(m_ffplayProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this, outPath](int, QProcess::ExitStatus) {
                            Q_UNUSED(outPath);
                            stopTTS();
                            // best-effort cleanup
                            QFile::remove(m_lastAudioPath);
                        });

                m_ffplayProc->start();
            });

    m_ttsProc->start();
}

