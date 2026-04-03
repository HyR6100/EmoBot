#pragma once

#include <QMainWindow>
#include <QColor>

class QLabel;
class QCheckBox;
class QProcess;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QTimer;
class QWidget;
class QPropertyAnimation;
class CompanionBackend;
class ChatInputEdit;
class PersonaDialog;
class QGroupBox;
class MjpegViewerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(CompanionBackend *backend, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSendClicked();
    void onAssistantReplyReady(const QString &reply, const QString &emotion, const QString &gesture);
    void onErrorOccurred(const QString &message);
    void onPersonaClicked();
    void onInputSendRequested(const QString &text);
    void onStatusChanged(const QString &status);
    void onPlayAudioClicked();

private:
    void appendMessage(bool isUser, const QString &text, const QString &emotionForAssistant = QString());
    void setBusy(bool busy);
    static QString emotionTail(const QString &emotion);
    static QColor assistantBubbleColor(const QString &emotion);
    void maybeAutoPlayTTS(const QString &text);
    void startTTS(const QString &text);
    void stopTTS();
    void pulseSpeaking(bool speaking);
    void updateUnreadUi();
    bool isScrolledToBottom() const;
    int distanceToBottom() const;
    void scrollToBottom();

private:
    CompanionBackend *m_backend = nullptr;

    QLabel *m_statusLabel = nullptr;
    QPushButton *m_sendBtn = nullptr;
    QPushButton *m_resetBtn = nullptr;
    QPushButton *m_personaBtn = nullptr;
    QCheckBox *m_autoPlayCheck = nullptr;
    QPushButton *m_playAudioBtn = nullptr;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_messagesContainer = nullptr;
    QVBoxLayout *m_messagesLayout = nullptr;
    QPushButton *m_newMsgBanner = nullptr;
    int m_unreadCount = 0;
    bool m_forceFollowAssistantOnce = false;

    ChatInputEdit *m_inputEdit = nullptr;

    QString m_currentPersonaText;
    PersonaDialog *m_personaDialog = nullptr;

    QGroupBox *m_videoPlaceholder = nullptr;
    QLabel *m_videoStatusLabel = nullptr;
    QString m_videoStatusDefaultText;
    MjpegViewerWidget *m_mjpegView = nullptr;

    bool m_busy = false;

    // Audio state
    bool m_ttsBusy = false;
    QString m_lastAssistantText;
    QString m_lastAudioPath;
    class QProcess *m_ttsProc = nullptr;
    class QProcess *m_ffplayProc = nullptr;
    QTimer *m_pulseTimer = nullptr;
    int m_pulsePhase = 0;
    QPropertyAnimation *m_scrollAnim = nullptr;
    QTimer *m_followLatestRetryTimer = nullptr;
    int m_followLatestRetryLeft = 0;
};

