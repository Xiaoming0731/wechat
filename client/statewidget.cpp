#include "statewidget.h"
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QEnterEvent>   // Qt6 必须引入，enterEvent 参数类型变更
#include <QDebug>
#include <QMouseEvent>

StateWidget::StateWidget(QWidget *parent)
    : QWidget(parent)
    , _curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
    AddRedPoint();
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);   // Qt6 改动：init() 已废弃，替换为 initFrom()
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

// 处理鼠标点击事件
void StateWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Selected){
            qDebug()<<"PressEvent , already to selected press: "<< _selected_press;
            QWidget::mousePressEvent(event);
            return;
        }

        if(_curstate == ClickLbState::Normal){
            qDebug()<<"PressEvent , change to selected press: "<< _selected_press;
            _curstate = ClickLbState::Selected;
            setProperty("state",_selected_press);
            repolish(this);
            update();
        }
        return;
    }
    QWidget::mousePressEvent(event);
}

void StateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
            setProperty("state",_normal_hover);
            repolish(this);
            update();
        }else{
            setProperty("state",_selected_hover);
            repolish(this);
            update();
        }
        emit clicked();
        return;
    }
    // 修复原代码bug：这里应该调用 mouseReleaseEvent，不是 mousePressEvent
    QWidget::mouseReleaseEvent(event);
}

// 处理鼠标悬停进入事件
void StateWidget::enterEvent(QEnterEvent* event)  // Qt6 改动：参数类型从 QEvent* 改为 QEnterEvent*
{
    if(_curstate == ClickLbState::Normal){
        setProperty("state",_normal_hover);
        repolish(this);
        update();
    }else{
        setProperty("state",_selected_hover);
        repolish(this);
        update();
    }
    QWidget::enterEvent(event);
}

// 处理鼠标悬停离开事件
void StateWidget::leaveEvent(QEvent* event)
{
    if(_curstate == ClickLbState::Normal){
        setProperty("state",_normal);
        repolish(this);
        update();
    }else{
        setProperty("state",_selected);
        repolish(this);
        update();
    }
    QWidget::leaveEvent(event);
}

void StateWidget::SetState(QString normal, QString hover, QString press,
                           QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    setProperty("state",normal);
    repolish(this);
}

ClickLbState StateWidget::GetCurState()
{
    return _curstate;
}

void StateWidget::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
}

void StateWidget::SetSelected(bool bselected)
{
    if(bselected){
        _curstate = ClickLbState::Selected;
        setProperty("state",_selected);
        repolish(this);
        update();
        return;
    }

    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
}

void StateWidget::AddRedPoint()
{
    _red_point = new QLabel();
    _red_point->setObjectName("red_point");
    QVBoxLayout* layout2 = new QVBoxLayout;
    _red_point->setAlignment(Qt::AlignCenter);
    layout2->addWidget(_red_point);
    layout2->setContentsMargins(0,0,0,0);  // Qt6 改动：setMargin 已移除，替换为 setContentsMargins
    this->setLayout(layout2);
    _red_point->setVisible(false);
}

void StateWidget::ShowRedPoint(bool show)
{
    // 修复原代码bug：参数 show 未生效，修正为根据参数控制显隐
    _red_point->setVisible(show);
}
