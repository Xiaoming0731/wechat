#include "friendapplywid.h"
#include "ui_friendapplywid.h"

#include "avatarutil.h"

FriendApplyWid::FriendApplyWid(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FriendApplyWid)
    , _reqid(0)
    , _handled(false)
{
    ui->setupUi(this);

    // 样式：纯白底 + hover变灰（和 friendwid/chatuserwid 保持一致）
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "FriendApplyWid{background-color:white;border-radius:12px;}"
        "FriendApplyWid:hover{background-color:rgb(200,200,200);border-radius:12px;}"));

    // 同意按钮 → 上报 reqid；已处理状态下不再发信号
    connect(ui->agree_btn, &QPushButton::clicked, this, [this]() {
        if (!_handled)
        {
            emit acceptClicked(_reqid);
        }
    });
}

FriendApplyWid::~FriendApplyWid()
{
    delete ui;
}

void FriendApplyWid::SetInfo(const ApplyInfo &info)
{
    _info = info;
    _reqid = info.reqid;
    _handled = false;

    ui->name_lb->setText(info.fromName);
    ui->text_lb->setText(info.applyMsg.isEmpty()
                         ? QStringLiteral("请求添加你为好友")
                         : info.applyMsg);
    ui->agree_btn->setText(QStringLiteral("同意"));
    ui->agree_btn->setEnabled(true);

    // 头像
    ui->icon_lb->setPixmap(AvatarUtil::pixmap(info.fromId, info.fromName, info.avatar));
    ui->icon_lb->setScaledContents(true);
}

void FriendApplyWid::markHandled()
{
    _handled = true;
    ui->agree_btn->setText(QStringLiteral("已同意"));
    ui->agree_btn->setEnabled(false);
}

void FriendApplyWid::markEnabled()
{
    _handled = false;
    ui->agree_btn->setText(QStringLiteral("同意"));
    ui->agree_btn->setEnabled(true);
}
