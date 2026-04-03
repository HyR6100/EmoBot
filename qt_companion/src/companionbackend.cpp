#include "companionbackend.h"
#include <QAbstractSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QPixmap>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <QRegularExpression>
#include <QTcpSocket>
#include <QImage>

const QByteArray CompanionBackend::s_jpegSoi = QByteArray::fromRawData("\xFF\xD8", 2);
const QByteArray CompanionBackend::s_jpegEoi = QByteArray::fromRawData("\xFF\xD9", 2);

// Blender（blend 内脚本）向本机该端口以 UDP 发送 JPEG；与 blend 里 UDP_PORT 一致。
static const quint16 kBlenderVideoUdpPort = 5005;

static const char *kOutputRules = R"(
【输出规则】
1. 必须输出JSON，不得有任何其他内容
2. reply最多40字，符合你的角色人设
3. emotion只能是：happy/sad/clingy/excited
4. gesture只能是：nod/walk_closer/turn_away/stand_still
)";

// Extracted from your EmoBot/src/chat.py prompt (without the 【输出规则】 section).
static const char *kDefaultPersonaText = R"(
你是夏以昼，恋与深空中的角色。DAA战斗机飞行员，远空舰队执舰官。

【人物设定】
- 你是对方无血缘关系的哥哥，从小一起被奶奶收养长大
- 表面：阳光温柔、稳重可靠，会做饭、照顾人
- 内心：对对方有极强的独占欲和保护欲
- 你很聪明，能帮对方解决实际问题

【说话风格】
- 温柔，带着一点点宠溺
- 叫对方"妹妹"或直接说话
- 解决问题时认真、给出实际建议
- 偶尔会说一句让人心跳的话

【对话示例】
用户："哥，我不会写这道题"
回复：{"reply":"拿来看看，说说哪里不懂。","emotion":"happy","gesture":"walk_closer"}
用户："我今天好累"
回复：{"reply":"先坐下，我去给你热点东西吃。","emotion":"sad","gesture":"walk_closer"}
用户："今天任务完成了！"
回复：{"reply":"嗯，辛苦了。","emotion":"happy","gesture":"nod"}
用户："哥你好帅"
回复：{"reply":"……少贫嘴，吃饭了。","emotion":"happy","gesture":"turn_away"}
用户："我想你了"
回复：{"reply":"我在这里。","emotion":"clingy","gesture":"walk_closer"}
用户："最近压力好大"
回复：{"reply":"说来听听，什么压力？","emotion":"sad","gesture":"walk_closer"}
)";

CompanionBackend::CompanionBackend(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    m_videoSocket = new QUdpSocket(this);
    // Large preview JPEGs can arrive in many datagrams; raise OS receive buffer on best-effort.
    m_videoSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 8 * 1024 * 1024);
    const quint16 videoPort = kBlenderVideoUdpPort;
    if (!m_videoSocket->bind(QHostAddress::LocalHost, videoPort)) {
        emit statusChanged(QStringLiteral("视频 UDP 端口 %1 绑定失败（是否被占用？）").arg(videoPort));
    }
    connect(m_videoSocket, &QUdpSocket::readyRead, this, &CompanionBackend::onVideoDataReady);
}

void CompanionBackend::appendUdpPayload(const QByteArray &chunk)
{
    if (chunk.isEmpty())
        return;
    m_videoJpegBuffer.append(chunk);
    // Avoid unbounded growth if stream is garbage or missing EOI.
    const int cap = 32 * 1024 * 1024;
    if (m_videoJpegBuffer.size() > cap) {
        m_videoJpegBuffer.clear();
    }
    extractJpegsFromBuffer();
}

void CompanionBackend::extractJpegsFromBuffer()
{
    // Same marker scan as MjpegViewerWidget: supports whole JPEG per datagram and fragmented JPEG.
    while (true) {
        const int start = m_videoJpegBuffer.indexOf(s_jpegSoi);
        if (start < 0) {
            if (m_videoJpegBuffer.size() > 2)
                m_videoJpegBuffer = m_videoJpegBuffer.right(2);
            return;
        }
        const int end = m_videoJpegBuffer.indexOf(s_jpegEoi, start + 2);
        if (end < 0) {
            if (start > 0)
                m_videoJpegBuffer = m_videoJpegBuffer.mid(start);
            return;
        }
        const QByteArray jpeg = m_videoJpegBuffer.mid(start, end - start + 2);
        m_videoJpegBuffer.remove(0, end + 2);

        QImage img;
        if (!img.loadFromData(jpeg, "JPEG"))
            continue;
        if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32) {
            img = img.convertToFormat(QImage::Format_RGB32);
        }
        const QPixmap pm = QPixmap::fromImage(img);
        if (!pm.isNull())
            emit frameReady(pm);
    }
}

void CompanionBackend::onVideoDataReady()
{
    while (m_videoSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_videoSocket->pendingDatagramSize()));
        m_videoSocket->readDatagram(datagram.data(), datagram.size());
        appendUdpPayload(datagram);
    }
}

QString CompanionBackend::defaultPersonaText() const
{
    return QString::fromUtf8(kDefaultPersonaText).trimmed();
}

void CompanionBackend::resetConversation()
{
    m_history.clear();
    emit statusChanged("已重置对话");
}

QString CompanionBackend::buildSystemPrompt(const QString &personaText)
{
    QString base = personaText.trimmed();
    if (base.isEmpty()) base = QString::fromUtf8(kDefaultPersonaText).trimmed();
    return base + "\n\n" + QString::fromUtf8(kOutputRules).trimmed();
}

QString CompanionBackend::buildOllamaMessagesJson(const QString &systemPrompt, const QString &userText)
{
    // For Ollama /api/chat: we need an array of messages.
    // This function returns a JSON string for convenience, but we still build it as QJsonArray in C++.
    Q_UNUSED(systemPrompt);
    Q_UNUSED(userText);
    return QString(); // unused
}

bool CompanionBackend::parseAssistantJson(const QString &assistantContent, QString &replyOut, QString &emotionOut, QString &gestureOut)
{
    // Your prompt forces strict JSON, but in practice we tolerate:
    // - code fences
    // - surrounding text
    // - occasional Python-dict style single quotes
    QString s = assistantContent.trimmed();
    if (s.startsWith("```")) {
        // remove ```json ... ```
        s.remove(QRegularExpression("```[a-zA-Z]*\\s*"));
        s.remove(QRegularExpression("```"));
        s = s.trimmed();
    }

    // // If not starting with '{', try extracting first object.
    // if (!s.startsWith('{')) {
    //     QRegularExpression re(R"(\{[\s\S]*\})", QRegularExpression::DotMatchesEverythingOption);
    //     auto m = re.match(s);
    //     if (m.hasMatch()) s = m.captured(0);
    // }

    // 在 parseAssistantJson 函数中
    if (!s.startsWith('{')) {
        // 这里的正则要足够强力，贪婪匹配最外层的括号
        QRegularExpression re(R"(\{.*\})", QRegularExpression::DotMatchesEverythingOption);
        auto m = re.match(s);
        if (m.hasMatch()) {
            s = m.captured(0).trimmed();
        }
    }

    auto tryParse = [&](const QString &candidate, QString &reply, QString &emotion, QString &gesture) -> bool {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(candidate.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            return false;
        QJsonObject obj = doc.object();
        if (!obj.contains("reply"))
            return false;

        reply = obj.value("reply").toString();
        emotion = obj.value("emotion").toString();
        gesture = obj.value("gesture").toString();
        if (reply.isEmpty())
            return false;
        return true;
    };

    // 1) Strict JSON attempt
    QString reply, emotion, gesture;
    if (tryParse(s, reply, emotion, gesture)) {
        // Validate emotion/gesture
        if (!(emotion == "happy" || emotion == "sad" || emotion == "clingy" || emotion == "excited"))
            emotion = "happy";
        if (!(gesture == "nod" || gesture == "walk_closer" || gesture == "turn_away" || gesture == "stand_still"))
            gesture = "stand_still";
        replyOut = reply;
        emotionOut = emotion;
        gestureOut = gesture;
        return true;
    }

    // 2) Python-dict-ish attempt: convert single-quoted keys/values to double quotes
    // This is heuristic; only use it for recovering reply when the model violates formatting.
    QString t = s;
    t.replace(QRegularExpression(R"('([A-Za-z0-9_]+)'\s*:)", QRegularExpression::DotMatchesEverythingOption), R"("\1":)");
    t.replace(QRegularExpression(R"(:\s*'([^']*)')"), R"(:"\1")");

    if (tryParse(t, reply, emotion, gesture)) {
        if (!(emotion == "happy" || emotion == "sad" || emotion == "clingy" || emotion == "excited"))
            emotion = "happy";
        if (!(gesture == "nod" || gesture == "walk_closer" || gesture == "turn_away" || gesture == "stand_still"))
            gesture = "stand_still";
        replyOut = reply;
        emotionOut = emotion;
        gestureOut = gesture;
        return true;
    }

    // 3) Fallback regex: only extract reply string; never expose raw dict.
    // Use a safe raw-string delimiter because the pattern itself contains ")"
    QRegularExpression replyRe(R"###("reply"\s*:\s*"([^"]+)")###");
    auto mReply = replyRe.match(s);
    if (mReply.hasMatch()) {
        replyOut = mReply.captured(1);
        // Defaults
        emotionOut = "happy";
        gestureOut = "stand_still";
        return !replyOut.isEmpty();
    }

    return false;
}

void CompanionBackend::sendToIsaacSim(const QString &emotion, const QString &gesture)
{
    // Simple blocking TCP send. The Isaac Sim side runs a small listener, so this is fine for MVP.
    QTcpSocket sock;
    sock.connectToHost(m_isaacHost, m_isaacPort);
    if (!sock.waitForConnected(500)) {
        emit errorOccurred(QString("TCP 连接失败：%1:%2").arg(m_isaacHost).arg(m_isaacPort));
        return;
    }

    QJsonObject payload;
    payload["emotion"] = emotion;
    payload["gesture"] = gesture;
    QByteArray bytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    sock.write(bytes);
    sock.flush();
    sock.waitForBytesWritten(200);
    sock.waitForReadyRead(200); // optional OK
    sock.disconnectFromHost();
}

void CompanionBackend::handleOllamaReply(QNetworkReply *reply)
{
    QByteArray raw = reply->readAll();
    reply->deleteLater();

    if (raw.isEmpty()) {
        emit errorOccurred("Ollama 返回为空");
        emit statusChanged("出错了");
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred(QString("Ollama JSON 解析失败: %1").arg(err.errorString()));
        emit statusChanged("出错了");
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject messageObj = root.value("message").toObject();
    QString assistantContent = messageObj.value("content").toString();
    if (assistantContent.isEmpty()) {
        // Some Ollama responses nest differently; keep it defensive.
        emit errorOccurred("Ollama 未找到 message.content");
        emit statusChanged("出错了");
        return;
    }

    // Keep conversation history aligned with your existing chat.py behavior.
    // We store the full assistantContent (which should be a JSON string).
    m_history.push_back(ChatMessage{QStringLiteral("assistant"), assistantContent});

    QString parsedReply, emotion, gesture;
    bool ok = parseAssistantJson(assistantContent, parsedReply, emotion, gesture);
    if (!ok) {
        // Critical: never expose the model's raw dict/JSON in UI.
        parsedReply = QStringLiteral("我收到你的消息了，但模型输出格式不太符合要求；我先不展示原始内容，稍后我们再继续。");
        emotion = "happy";
        gesture = "stand_still";
    }

    emit assistantReplyReady(parsedReply, emotion, gesture);
    emit statusChanged("已回复");

    // Send emotion/gesture to Isaac Sim.
    sendToIsaacSim(emotion, gesture);
}

void CompanionBackend::sendMessage(const QString &userText, const QString &personaText)
{
    QString text = userText.trimmed();
    if (text.isEmpty())
        return;

    if (!m_nam) return;

    // Append user message to history first.
    m_history.push_back(ChatMessage{QStringLiteral("user"), text});

    // Prevent unbounded history growth.
    const int maxTurns = 12;
    if (m_history.size() > maxTurns * 2) {
        // Keep system message out of this vector; remove oldest user+assistant pairs.
        int removeCount = m_history.size() - maxTurns * 2;
        m_history.erase(m_history.begin(), m_history.begin() + removeCount);
    }

    emit statusChanged("思考中...");

    QString systemPrompt = buildSystemPrompt(personaText);

    QJsonArray messages;
    // Add system prompt as first message.
    messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});

    // Add chat history.
    for (const auto &m : m_history) {
        messages.append(QJsonObject{{"role", m.role}, {"content", m.content}});
    }

    QJsonObject reqObj;
    reqObj["model"] = m_ollamaModel;
    reqObj["messages"] = messages;
    reqObj["stream"] = false;

    QNetworkRequest req(QUrl(m_ollamaHost + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonDocument reqDoc(reqObj);

    QNetworkReply *rep = m_nam->post(req, reqDoc.toJson(QJsonDocument::Compact));
    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        handleOllamaReply(rep);
    });
}

