#include "chatinputedit.h"

#include <QKeyEvent>

ChatInputEdit::ChatInputEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setAcceptRichText(false);
}

void ChatInputEdit::keyPressEvent(QKeyEvent *e)
{
    if (!e) return;

    const bool isReturn = (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter);
    const bool hasShift = (e->modifiers() & Qt::ShiftModifier);

    // Enter sends; Shift+Enter inserts newline.
    if (isReturn && !hasShift) {
        const QString text = toPlainText().trimmed();
        if (!text.isEmpty()) {
            emit sendRequested(text);
            clear();
        }
        e->accept();
        return;
    }

    QTextEdit::keyPressEvent(e);
}

