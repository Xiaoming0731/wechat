#include "chatuserwid.h"
#include "ui_chatuserwid.h"

#include <QStyle>
#include <QDateTime>
#include <QResizeEvent>

ChatUserWid::ChatUserWid(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::ChatUserWid),
    _unread(false),
    _redPoint(nullptr)
{
    ui->setupUi(this);
    SetItemType(ListItemType::CHAT_USER_ITEM);

    // 和 friendwid 一样，背景完全由控件自己画：
    // 显式写类选择器，避免规则级联到子控件（否则头像/文字 QLabel 会盖上不透明底色）
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "ChatUserWid{background-color:rgb(255,255,255);}"
        "ChatUserWid:hover{background-color:rgb(235,235,235);}"
        "ChatUserWid[selected=\"true\"]{background-color:rgb(210,210,210);}"));

    // 红点：比侧边栏的 18×18 略小，14×14 正好压在 icon_lb（45×45）的右上角边缘
    // 用 QSS 画纯色圆，不用 red_point.png（那张图自带白底，会把头像右上角糊掉）
    _redPoint = new QLabel(this);
    _redPoint->setFixedSize(14, 14);
    _redPoint->setStyleSheet(QStringLiteral(
        "background-color: rgb(250,81,81); border-radius: 7px;"));
    _redPoint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    _redPoint->setVisible(false);
}

ChatUserWid::~ChatUserWid()
{
    delete ui;
}

void ChatUserWid::setSelected(bool on)
{
    if (property("selected").toBool() == on)
        return;

    setProperty("selected", on);
    // 动态属性变化后 QSS 不会自动重算，需要 unpolish/polish 一次
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ChatUserWid::SetInfo(int friendId, const QString &name, const QPixmap &head, const QString &msg,
                          bool isGroup)
{
    if (isGroup)
    {
        _groupId = friendId;
        _friendId = 0;
    }
    else
    {
        _groupId = 0;
        _friendId = friendId;
    }
    _name = name;
    _msg = msg;

    ui->icon_lb->setPixmap(head.scaled(ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->name_lb->setText(_name);
    ui->user_chat_lb->setText(_msg);
}

void ChatUserWid::setChatText(const QString &msg)
{
    _msg = msg;
    ui->user_chat_lb->setText(msg);
}

void ChatUserWid::setDisplayName(const QString &name)
{
    if (_name == name) { return; }
    _name = name;
    ui->name_lb->setText(name);
}

void ChatUserWid::setTime(const QString &time)
{
    if (time.isEmpty())
    {
        ui->time_lb->clear();
        return;
    }

    const QDateTime dt = QDateTime::fromString(time, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dt.isValid())
    {
        ui->time_lb->setText(time);
        return;
    }

    const QDate today = QDate::currentDate();
    if (dt.date() == today)
    {
        ui->time_lb->setText(dt.toString(QStringLiteral("HH:mm")));
    }
    else if (dt.date() == today.addDays(-1))
    {
        ui->time_lb->setText(QStringLiteral("昨天"));
    }
    else
    {
        ui->time_lb->setText(dt.toString(QStringLiteral("MM-dd")));
    }
}

void ChatUserWid::setUnread(bool on)
{
    if (_unread == on) { return; }
    _unread = on;
    _redPoint->setVisible(on);
    // 关键：不能靠 resize(size()) 触发 resizeEvent —— 尺寸没变时 Qt 不会发这个事件，
    // 红点会停在默认 (0,0) 被头像盖住。这里直接同步定位一次。
    if (on)
    {
        updateRedPointGeometry();
    }
}

void ChatUserWid::updateRedPointGeometry()
{
    if (!_redPoint) { return; }

    // 红点始终是 ChatUserWid 的子控件（不搬到外层），这样会话列表滚动时红点会跟着行走，
    // 也不会被 QListWidget 的 viewport 重排搞错位。
    // icon_lb 位于 ChatUserWid 内部，右上角再向外溢出 7px 仍在自身矩形内，不会被裁剪。
    const QPoint iconTopRight = ui->icon_lb->mapTo(this, QPoint(ui->icon_lb->width(), 0));
    _redPoint->move(iconTopRight.x() - 7, iconTopRight.y() - 7);
    _redPoint->raise();   // 保证压在头像/文字之上
}

void ChatUserWid::resizeEvent(QResizeEvent *event)
{
    ListItemBase::resizeEvent(event);
    updateRedPointGeometry();
}
