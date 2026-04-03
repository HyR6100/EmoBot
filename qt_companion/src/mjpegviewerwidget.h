#pragma once

#include <QWidget>

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

private:
    void parseAndUpdateFrames();
    void updatePixmapFromJpeg(const QByteArray &jpeg);

private:
    QLabel *m_label = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QByteArray m_buffer;
    bool m_hasStream = false;

    // JPEG markers
    static const QByteArray SOI;
    static const QByteArray EOI;
};

