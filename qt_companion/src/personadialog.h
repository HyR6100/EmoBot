#pragma once

#include <QDialog>

class QComboBox;
class QTextEdit;

class PersonaDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PersonaDialog(const QString &basePersona, QWidget *parent = nullptr);

    QString personaText() const;

private:
    void syncCustomModeUi();

private:
    QString m_basePersona;
    QComboBox *m_combo = nullptr;
    QTextEdit *m_customEdit = nullptr;
};

