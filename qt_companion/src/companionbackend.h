#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QUdpSocket>
#include <QByteArray>
#include <QPixmap>
#include <QHostAddress>

class QNetworkAccessManager;
class QNetworkReply;

class CompanionBackend : public QObject
{
    Q_OBJECT
public:
    explicit CompanionBackend(QObject *parent = nullptr);

    Q_INVOKABLE void resetConversation();
    Q_INVOKABLE QString defaultPersonaText() const;
    Q_INVOKABLE void sendMessage(const QString &userText, const QString &personaText);

signals:
    void assistantReplyReady(const QString &reply, const QString &emotion, const QString &gesture);
    void statusChanged(const QString &status);
    void errorOccurred(const QString &message);
    void frameReady(const QPixmap &pix);

private slots:
    void onVideoDataReady();

private:
    struct ChatMessage {
        QString role;   // "user" | "assistant"
        QString content;
    };

    void sendToIsaacSim(const QString &emotion, const QString &gesture);
    void handleOllamaReply(QNetworkReply *reply);
    void appendUdpPayload(const QByteArray &chunk);
    void extractJpegsFromBuffer();
    static bool parseAssistantJson(const QString &assistantContent, QString &replyOut, QString &emotionOut, QString &gestureOut);
    static QString buildSystemPrompt(const QString &personaText);
    QString buildOllamaMessagesJson(const QString &systemPrompt, const QString &userText);

private:
    QNetworkAccessManager *m_nam = nullptr;
    QVector<ChatMessage> m_history;

    // Defaults (you can later expose them via UI/settings)
    QString m_ollamaHost = "http://localhost:11434";
    QString m_ollamaModel = "qwen2.5:14b";

    QString m_isaacHost = "127.0.0.1";
    quint16 m_isaacPort = 12345;
    QUdpSocket *m_videoSocket = nullptr;
    QByteArray m_videoJpegBuffer;
    static const QByteArray s_jpegSoi;
    static const QByteArray s_jpegEoi;
};

