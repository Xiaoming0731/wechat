#include "IconSwitchBtn.h"

#include <QResizeEvent>

IconSwitchBtn::IconSwitchBtn(QWidget *parent)
    : QPushButton(parent)
{
    setFlat(true);
    setCursor(Qt::PointingHandCursor);
    setCheckable(true);

    // 红点 18×18，父对象先设成 this，等第一次 resizeEvent 时再切到 parentWidget()
    // （IconSwitchBtn 本身就是 30×30 icon 大小，红点必须溢出到它外面才不会遮挡图标）
    m_redPoint = new QLabel(this);
    m_redPoint->setObjectName("red_point");
    m_redPoint->setFixedSize(18, 18);
    // 用 QSS 画纯色圆，不用 red_point.png：那张图自带白色底，
    // 会以 18×18 白方块的形式把图标右上角糊掉，看起来像"红点之外的部分遮挡"
    m_redPoint->setStyleSheet(QStringLiteral(
        "background-color: rgb(250,81,81); border-radius: 9px;"));
    // 不拦截鼠标，避免挡住按钮点击
    m_redPoint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_redPoint->setVisible(false);
}

void IconSwitchBtn::relocateRedPoint()
{
    if (!m_redPoint) { return; }

    // 把红点"搬家"到外层父容器（widget_2），这样它可以溢出 IconSwitchBtn 边界
    QWidget *p = parentWidget();
    if (!p) { return; }

    if (m_redPoint->parent() != p)
    {
        // setParent 会强制隐藏控件，必须用 m_redVisible 兜底恢复，
        // 否则"先 ShowRedPoint(true) 再 resizeEvent"的路径会把红点永久藏起来
        m_redPoint->setParent(p);
    }

    // 按钮右上角坐标 → 红点 18×18 压在按钮边界顶点上：
    //   mapTo 把按钮右上角 (width,0) 映射到父容器坐标系
    //   红点左上角 = (btnRight - 9, btnTop - 9)，红点中心正好卡按钮右上角边界
    const QPoint btnTopRight = mapTo(p, QPoint(width(), 0));
    m_redPoint->move(btnTopRight.x() - 9, btnTopRight.y() - 9);
    m_redPoint->setVisible(m_redVisible);
    if (m_redVisible)
    {
        m_redPoint->raise();   // 保证压在后加入/重排的兄弟控件之上
    }
}

void IconSwitchBtn::resizeEvent(QResizeEvent *event)
{
    QPushButton::resizeEvent(event);
    relocateRedPoint();
}


void IconSwitchBtn::setIcons(const QString &normal,
                             const QString &hover,
                             const QString &press,
                             const QString &checked,
                             const QString &checkedHover,
                             const QString &checkedPress)
{
    QString qss;
    qss += QString(R"(
    QPushButton{
        border: none;
        background: transparent;
        border-image: url(%1);
    }
    )").arg(normal);

    if(!hover.isEmpty())
    {
        qss += QString(R"(QPushButton:hover{border-image: url(%1);})").arg(hover);
    }
    if(!press.isEmpty())
    {
        qss += QString(R"(QPushButton:pressed{border-image: url(%1);})").arg(press);
    }
    if(!checked.isEmpty())
    {
        qss += QString(R"(QPushButton:checked{border-image: url(%1);})").arg(checked);
    }
    if(!checkedHover.isEmpty())
    {
        qss += QString(R"(QPushButton:checked:hover{border-image: url(%1);})").arg(checkedHover);
    }
    if(!checkedPress.isEmpty())
    {
        qss += QString(R"(QPushButton:checked:pressed{border-image: url(%1);})").arg(checkedPress);
    }
    setStyleSheet(qss);
}


void IconSwitchBtn::setIconSize(const QSize &sz)
{
    setStyleSheet(styleSheet() + QString("icon-size: %1px %2px;").arg(sz.width()).arg(sz.height()));
}

void IconSwitchBtn::ShowRedPoint(bool show)
{
    m_redVisible = show;
    // 先定位再显示：首次显示时 resizeEvent 可能还早于本调用，
    // 直接 setVisible 会停在 (0,0) 或被 setParent 隐藏
    relocateRedPoint();
}
