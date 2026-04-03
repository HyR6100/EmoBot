#pragma once

#include <QWidget>
#include <QPixmap>
#include <QResizeEvent>

class QLabel;
class QNetworkAccessManager;
class QNetworkReply;
class QByteArray;

class MjpegViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MjpegViewerWidget(QWidget *parent = nullptr);
    ~MjpegViewerWidget() override;

    void start(const QString &url);
    void stop();
    // Blender / UDP path: show frames pushed from CompanionBackend (or tests).
    void showFrame(const QPixmap &pix);

private slots:
    void onReplyReadyRead();
    void onReplyFinished();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void parseAndUpdateFrames();
    void updatePixmapFromJpeg(const QByteArray &jpeg);
    void setSourceFrame(const QPixmap &pix);
    void refreshLabelPixmap();

private:
    QLabel *m_label = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QByteArray m_buffer;
    bool m_hasStream = false;
    QPixmap m_lastSource;

    // JPEG markers
    static const QByteArray SOI;
    static const QByteArray EOI;
};

