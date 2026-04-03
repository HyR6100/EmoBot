#pragma once

#include <QTextEdit>

class ChatInputEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ChatInputEdit(QWidget *parent = nullptr);

signals:
    void sendRequested(const QString &text);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

