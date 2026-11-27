#include "ChatView.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QDebug>
#include <QTimer>
#include <QStyleOption>
#include <QPainter>
#include <QPaintEvent>   // Qt6建议显式引入

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
    , isAppended(false)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    this->setLayout(pMainLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 0); // Qt6: setMargin已移除，改用setContentsMargins

    m_pScrollArea = new QScrollArea();
    m_pScrollArea->setObjectName("chat_area");
    pMainLayout->addWidget(m_pScrollArea);

    QWidget *w = new QWidget(this);
    w->setObjectName("chat_bg");
    w->setAutoFillBackground(true);
    QVBoxLayout *pHLayout_1 = new QVBoxLayout();
    pHLayout_1->addWidget(new QWidget(), 100000); // 底部弹簧，消息靠下排列
    pHLayout_1->setContentsMargins(0, 0, 0, 0);
    w->setLayout(pHLayout_1);
    m_pScrollArea->setWidget(w);

    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
    connect(pVScrollBar, &QScrollBar::rangeChanged, this, &ChatView::onVScrollBarMoved);

    // 滚动条悬浮在右侧，不占用内容宽度
    QHBoxLayout *pHLayout_2 = new QHBoxLayout();
    pHLayout_2->addWidget(pVScrollBar, 0, Qt::AlignRight);
    pHLayout_2->setContentsMargins(0, 0, 0, 0);
    m_pScrollArea->setLayout(pHLayout_2);
    pVScrollBar->setHidden(true);

    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->installEventFilter(this);
    initStyleSheet();
}

void ChatView::appendChatItem(QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    vl->insertWidget(vl->count() - 1, item); // 插入到弹簧之前
    isAppended = true;
}

void ChatView::prependChatItem(QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    vl->insertWidget(0, item);
}

void ChatView::insertChatItem(QWidget *before, QWidget *item)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    int index = vl->indexOf(before);
    if (index >= 0) {
        vl->insertWidget(index, item);
    }
}

void ChatView::removeAllItem()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(m_pScrollArea->widget()->layout());
    int count = layout->count();
    // 保留最后一个弹簧widget，只删除消息项
    for (int i = 0; i < count - 1; ++i) {
        QLayoutItem *item = layout->takeAt(0);
        if (item) {
            if (QWidget *widget = item->widget()) {
                widget->deleteLater();
            }
            delete item;
        }
    }
}

bool ChatView::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Enter && o == m_pScrollArea)
    {
        // 内容不足一屏时不显示滚动条
        m_pScrollArea->verticalScrollBar()->setHidden(
            m_pScrollArea->verticalScrollBar()->maximum() == 0);
    }
    else if (e->type() == QEvent::Leave && o == m_pScrollArea)
    {
        m_pScrollArea->verticalScrollBar()->setHidden(true);
    }
    return QWidget::eventFilter(o, e);
}

void ChatView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this); // Qt6: init()已废弃，改用initFrom
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void ChatView::onVScrollBarMoved(int min, int max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);
    if (isAppended)
    {
        QScrollBar *pVScrollBar = m_pScrollArea->verticalScrollBar();
        pVScrollBar->setSliderPosition(pVScrollBar->maximum());
        // 防抖：500ms内多次追加只滚一次底
        QTimer::singleShot(500, [this]()
                           {
                               isAppended = false;
                           });
    }
}

void ChatView::initStyleSheet()
{
    // 可在此处加载滚动条样式
}
