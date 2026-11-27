#include "friendpage.h"
#include "ui_friendpage.h"
#include "avatarutil.h"

FriendPage::FriendPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FriendPage)
{
    ui->setupUi(this);
    ui->chat_btn->setIcons(
        ":/new/prefix1/res/msg_chat_normal.png",        // 未选中默认
        ":/new/prefix1/res/msg_chat_hover.png",         // 未选中悬浮
        ":/new/prefix1/res/msg_chat_press.png",         // 未选中按下
        ":/new/prefix1/res/msg_chat_normal.png",      // 选中默认
        ":/new/prefix1/res/msg_chat_hover.png",// 选中悬浮
        ":/new/prefix1/res/msg_chat_press.png" // 选中按下
        );
    ui->video_btn->setIcons(
        ":/new/prefix1/res/video_chat_normal.png",        // 未选中默认
        ":/new/prefix1/res/video_chat_hover.png",         // 未选中悬浮
        ":/new/prefix1/res/video_chat_press.png",         // 未选中按下
        ":/new/prefix1/res/video_chat_normal.png",      // 选中默认
        ":/new/prefix1/res/video_chat_hover.png",// 选中悬浮
        ":/new/prefix1/res/video_chat_press.png" // 选中按下
        );
    ui->setting_btn->setIcons(
        ":/new/prefix1/res/settings.png",
        ":/new/prefix1/res/settings_hover.png",
        ":/new/prefix1/res/settings_press.png",
        ":/new/prefix1/res/settings.png",
        ":/new/prefix1/res/settings_select_hover.png",
        ":/new/prefix1/res/settings_select_press.png"
        );
    ui->setting_btn->setFixedSize(25,25);

    // 设置按钮 → 弹"改好友备注"
    connect(ui->setting_btn, &IconSwitchBtn::clicked, this, [this]() {
        emit settingClicked(_friend.id, _friend.name, _friend.remark);
    });
}

FriendPage::~FriendPage()
{
    delete ui;
}

void FriendPage::SetFriend(const FriendInfo &info)
{
    _friend = info;

    // 头像（friendpage.ui 里的 label）
    ui->label->setPixmap(AvatarUtil::pixmap(info.id, info.name, info.avatar));
    ui->label->setScaledContents(true);

    // 备注：有备注显示备注，没有显示原昵称；都在这一行，底下 nickname 显示原昵称
    const QString displayName = info.remark.isEmpty() ? info.name : info.remark;
    ui->remark_lb->setText(displayName);
    ui->name_lb->setText(info.name);
    ui->id_lb->setText(QString::number(info.id));
}

void FriendPage::on_chat_btn_clicked()
{
    emit chatButtonClicked(_friend.id);
}

