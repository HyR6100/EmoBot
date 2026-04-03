#include "mjpegviewerwidget.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QImage>
#include <QVBoxLayout>
#include <QtGlobal>

void MjpegViewerWidget::showFrame(const QPixmap &pix)
{
    if (!m_label || pix.isNull())
        return;
    m_hasStream = true;
    const QSize target = m_label->size();
    if (target.isValid() && target.width() > 10 && target.height() > 10) {
        m_label->setPixmap(pix.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_label->setPixmap(pix);
    }
    m_label->setText(QString());
}

const QByteArray MjpegViewerWidget::SOI = QByteArray::fromRawData("\xFF\xD8", 2);
const QByteArray MjpegViewerWidget::EOI = QByteArray::fromRawData("\xFF\xD9", 2);

MjpegViewerWidget::MjpegViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setText(QStringLiteral("MJPEG 流未开始"));
    m_label->setStyleSheet("color:#6b3d86;");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_label, 1);
}

MjpegViewerWidget::~MjpegViewerWidget()
{
    stop();
}

void MjpegViewerWidget::start(const QString &url)
{
    stop();

    m_hasStream = false;
    m_buffer.clear();

    m_nam = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "EmobotQtCompanion");
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif

    m_reply = m_nam->get(req);
    connect(m_reply, &QNetworkReply::readyRead, this, &MjpegViewerWidget::onReplyReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &MjpegViewerWidget::onReplyFinished);
}

void MjpegViewerWidget::stop()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_nam) {
        delete m_nam;
        m_nam = nullptr;
    }
    m_buffer.clear();
    if (m_label) {
        m_label->setText(QStringLiteral("MJPEG 流已停止"));
    }
}

void MjpegViewerWidget::onReplyFinished()
{
    if (!m_hasStream && m_label) {
        if (m_reply && m_reply->error() != QNetworkReply::NoError) {
            m_label->setText(QStringLiteral("MJPEG：%1").arg(m_reply->errorString()));
        } else {
            m_label->setText(QStringLiteral("MJPEG：连接结束（无画面）"));
        }
    }
}

void MjpegViewerWidget::onReplyReadyRead()
{
    if (!m_reply) return;
    const QByteArray chunk = m_reply->readAll();
    if (chunk.isEmpty()) return;

    m_buffer.append(chunk);
    parseAndUpdateFrames();
}

void MjpegViewerWidget::parseAndUpdateFrames()
{
    // Robust parsing: search JPEG SOI/EOI markers in the stream.
    while (true) {
        const int start = m_buffer.indexOf(SOI);
        if (start < 0) {
            // Keep tail bytes
            if (m_buffer.size() > 2) {
                m_buffer = m_buffer.right(2);
            }
            return;
        }

        const int end = m_buffer.indexOf(EOI, start + 2);
        if (end < 0) {
            // wait for more data
            if (start > 0) {
                m_buffer = m_buffer.mid(start);
            }
            return;
        }

        const QByteArray jpeg = m_buffer.mid(start, end - start + 2);
        // Remove everything up to end+2
        m_buffer.remove(0, end + 2);

        updatePixmapFromJpeg(jpeg);
        m_hasStream = true;
    }
}

void MjpegViewerWidget::updatePixmapFromJpeg(const QByteArray &jpeg)
{
    QImage img;
    img.loadFromData(jpeg, "JPG");
    if (img.isNull())
        return;
    // Scale by keeping aspect ratio.
    const QPixmap px = QPixmap::fromImage(img);
    if (m_label) {
        const QSize target = m_label->size();
        if (target.isValid()) {
            m_label->setPixmap(px.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_label->setPixmap(px);
        }
        m_label->setText(QString());
    }
}

