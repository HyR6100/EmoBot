#include "personadialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>

PersonaDialog::PersonaDialog(const QString &basePersona, QWidget *parent)
    : QDialog(parent)
    , m_basePersona(basePersona)
{
    setWindowTitle(QStringLiteral("人物性格（选择/自定义）"));
    resize(620, 420);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    QLabel *title = new QLabel(QStringLiteral("选择一个陪伴风格，或自定义补充文本。"));
    title->setStyleSheet("color:#ffd1ff;font-weight:700;");
    layout->addWidget(title);

    m_combo = new QComboBox(this);
    m_combo->addItem(QStringLiteral("夏以昼-默认哥哥模式"), QVariant(0));
    m_combo->addItem(QStringLiteral("温柔治愈（更关心/更轻声）"), QVariant(1));
    m_combo->addItem(QStringLiteral("撒娇黏人（更 clingy）"), QVariant(2));
    m_combo->addItem(QStringLiteral("兴奋热情（更有活力）"), QVariant(3));
    m_combo->addItem(QStringLiteral("自定义补充文本"), QVariant(4));
    layout->addWidget(m_combo);

    m_customEdit = new QTextEdit(this);
    m_customEdit->setAcceptRichText(false);
    m_customEdit->setPlaceholderText(QStringLiteral("只写“补充风格/说话习惯”即可，例如：更温柔、更短句、更爱叫妹妹。"));
    m_customEdit->setEnabled(false);
    layout->addWidget(m_customEdit, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    connect(m_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PersonaDialog::syncCustomModeUi);

    syncCustomModeUi();
}

void PersonaDialog::syncCustomModeUi()
{
    // last item is custom
    const int idx = m_combo->currentIndex();
    const bool customMode = (idx == 4);
    m_customEdit->setEnabled(customMode);
}

QString PersonaDialog::personaText() const
{
    const int idx = m_combo->currentIndex();

    QString extra;
    if (idx == 0) {
        extra = QString();
    } else if (idx == 1) {
        extra = QStringLiteral("补充风格：更温柔、更治愈，语气更轻、更像在哄人；多用关心与安抚。\n");
    } else if (idx == 2) {
        extra = QStringLiteral("补充风格：更撒娇黏人，更喜欢说“我在”，用更亲昵的称呼与更频繁的靠近暗示。\n");
    } else if (idx == 3) {
        extra = QStringLiteral("补充风格：更兴奋热情，回复节奏更快，常带一点鼓励与闪耀感。\n");
    } else {
        extra = m_customEdit->toPlainText().trimmed();
        extra += (extra.isEmpty() ? QString() : QStringLiteral("\n"));
    }

    return m_basePersona + (extra.isEmpty() ? QString() : "\n" + extra);
}

