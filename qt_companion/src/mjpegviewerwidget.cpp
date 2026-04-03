#include "mjpegviewerwidget.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QImage>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QTimer>
#include <QtGlobal>

void MjpegViewerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshLabelPixmap();
}

void MjpegViewerWidget::setSourceFrame(const QPixmap &pix)
{
    if (pix.isNull())
        return;
    m_lastSource = pix;
    m_hasStream = true;
    refreshLabelPixmap();
    // 嵌套在 QGroupBox 里时，首帧时常尚未完成布局，多拍刷新避免 label 尺寸仍为 0
    QTimer::singleShot(0, this, [this]() { refreshLabelPixmap(); });
    QTimer::singleShot(50, this, [this]() { refreshLabelPixmap(); });
    QTimer::singleShot(200, this, [this]() { refreshLabelPixmap(); });
}

void MjpegViewerWidget::refreshLabelPixmap()
{
    if (!m_label || m_lastSource.isNull())
        return;

    QRect lr = m_label->contentsRect();
    int tw = lr.width();
    int th = lr.height();

    // 布局还没给 QLabel 有效区域时：用控件宽度推算，避免永远不 setPixmap
    if (tw <= 2 || th <= 2) {
        const int w = qMax(280, contentsRect().width() - 20);
        const double aspect = double(m_lastSource.height()) / qMax(1, m_lastSource.width());
        tw = w;
        th = qMax(100, int(qRound(w * aspect)));
    }

    const QPixmap scaled = m_lastSource.scaled(tw, th, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_label->setPixmap(scaled);
    m_label->setText(QString());
    m_label->update();
}

void MjpegViewerWidget::showFrame(const QPixmap &pix)
{
    if (!m_label || pix.isNull())
        return;
    setSourceFrame(pix);
}

const QByteArray MjpegViewerWidget::SOI = QByteArray::fromRawData("\xFF\xD8", 2);
const QByteArray MjpegViewerWidget::EOI = QByteArray::fromRawData("\xFF\xD9", 2);

MjpegViewerWidget::MjpegViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setMinimumSize(300, 168);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setText(QStringLiteral("MJPEG / Blender UDP 预览"));
    m_label->setStyleSheet(
        "color:#6b3d86; background: rgba(255,255,255,0.35); border-radius: 8px;");
    m_label->setScaledContents(false);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_label->setMinimumSize(280, 120);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);
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
    m_lastSource = QPixmap();
    if (m_label) {
        m_label->clear();
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
    while (true) {
        const int start = m_buffer.indexOf(SOI);
        if (start < 0) {
            if (m_buffer.size() > 2) {
                m_buffer = m_buffer.right(2);
            }
            return;
        }

        const int end = m_buffer.indexOf(EOI, start + 2);
        if (end < 0) {
            if (start > 0) {
                m_buffer = m_buffer.mid(start);
            }
            return;
        }

        const QByteArray jpeg = m_buffer.mid(start, end - start + 2);
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
    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32) {
        img = img.convertToFormat(QImage::Format_RGB32);
    }
    setSourceFrame(QPixmap::fromImage(img));
}
