#include "TextBubble.h"
#include <QFontMetricsF>
#include <QDebug>
#include <QFont>
#include "global.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QTextOption>
#include <QEvent>

TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    : BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setFrameShape(QFrame::NoFrame); // 彻底去掉原生边框，避免占用宽度
    m_pTextEdit->installEventFilter(this);

    QFont font("Microsoft YaHei");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);

    // 核心：文本任意位置可断行，英文数字和中文表现一致
    QTextOption opt = m_pTextEdit->document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAnywhere);
    m_pTextEdit->document()->setDefaultTextOption(opt);

    setPlainText(text);
    setWidget(m_pTextEdit);
    initStyleSheet();
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    if (m_pTextEdit == o && e->type() == QEvent::Paint)
    {
        adjustTextHeight();
    }
    return BubbleFrame::eventFilter(o, e);
}

void TextBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text);

    qreal doc_margin = m_pTextEdit->document()->documentMargin();
    int margin_left = this->layout()->contentsMargins().left();
    int margin_right = this->layout()->contentsMargins().right();

    QFontMetricsF fm(m_pTextEdit->font());
    QTextDocument *doc = m_pTextEdit->document();
    int max_width = 0;

    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        int txtW = int(fm.horizontalAdvance(it.text()));
        max_width = max_width < txtW ? txtW : max_width;
    }

    // +6px 宽度冗余，抵消字体渲染、控件边距的细微误差，避免卡边换行
    const int extraWidth = 6;
    setMaximumWidth(max_width + doc_margin * 2 + margin_left + margin_right + extraWidth);
}

void TextBubble::adjustTextHeight()
{
    qreal doc_margin = m_pTextEdit->document()->documentMargin();
    QTextDocument *doc = m_pTextEdit->document();
    qreal text_height = 0;

    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next())
    {
        QTextLayout *pLayout = it.layout();
        QRectF text_rect = pLayout->boundingRect();
        text_height += text_rect.height();
    }

    int vMargin = this->layout()->contentsMargins().top();
    setFixedHeight(text_height + doc_margin * 2 + vMargin * 2);
}

void TextBubble::initStyleSheet()
{
    m_pTextEdit->setStyleSheet(R"(
        QTextEdit{
            background: transparent;
            border: none;
            outline: none;
            color: #333333;
            padding: 0px;
        }
    )");
}
