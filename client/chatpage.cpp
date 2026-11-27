#include "chatitembase.h"
#include "chatpage.h"
#include "chatservice.h"
#include "avatarutil.h"
#include "global.h"
#include "picturebubble.h"
#include "textbubble.h"
#include "ui_chatpage.h"

#include <QDateTime>

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatPage)
    , _friendId(0)
{
    ui->setupUi(this);

    ui->emo_btn->setIcons(
        ":/new/prefix1/res/smile.png",
        ":/new/prefix1/res/smile_hover.png",
        ":/new/prefix1/res/smile_press.png",
        ":/new/prefix1/res/smile.png",
        ":/new/prefix1/res/smile_hover.png",
        ":/new/prefix1/res/smile_press.png"
        );
    ui->emo_btn->setFixedSize(24,24);

    ui->file_btn->setIcons(
        ":/new/prefix1/res/filedir.png",
        ":/new/prefix1/res/filedir_hover.png",
        ":/new/prefix1/res/filedir_press.png",
        ":/new/prefix1/res/filedir.png",
        ":/new/prefix1/res/filedir_hover.png",
        ":/new/prefix1/res/filedir_press.png"
        );
    ui->file_btn->setFixedSize(24,24);

    ui->setting_btn->setIcons(
        ":/new/prefix1/res/settings.png",
        ":/new/prefix1/res/settings_hover.png",
        ":/new/prefix1/res/settings_press.png",
        ":/new/prefix1/res/settings.png",
        ":/new/prefix1/res/settings_select_hover.png",
        ":/new/prefix1/res/settings_select_press.png"
        );
    ui->setting_btn->setFixedSize(25,25);

    // 设置按钮 → 把当前上下文吐出去让 ChatDialog 弹 SetRemark
    connect(ui->setting_btn, &IconSwitchBtn::clicked, this, [this]() {
        if (_isGroup)
        {
            emit settingClicked(0, _groupId, _group.name, _group.remark);
        }
        else
        {
            emit settingClicked(_friendId, 0, _friend.name, _friend.remark);
        }
    });
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::SetFriend(const FriendInfo &info)
{
    _friend = info;
    _friendId = info.id;
    _groupId = 0;
    _isGroup = false;
    _group = GroupInfo{};
    // 填 title_wid 里的 name_lb（备注优先，无备注显示原昵称）
    ui->chat_name_lb->setText(info.displayName());
}

void ChatPage::SetGroup(const GroupInfo &info)
{
    _group = info;
    _groupId = info.id;
    _isGroup = true;
    _friendId = 0;
    _friend = FriendInfo{};
    ui->chat_name_lb->setText(info.displayName());
}

void ChatPage::clearMessages()
{
    ui->chat_data_list->removeAllItem();
}

void ChatPage::appendMessage(const ChatMessage &msg)
{
    ChatService *service = ChatService::instance();
    const bool self = (msg.fromId == service->userId());
    const ChatRole role = self ? ChatRole::Self : ChatRole::Other;

    ChatItemBase *pChatItem = new ChatItemBase(role);
    if (self)
    {
        pChatItem->setUserName(service->userName());
        pChatItem->setUserIcon(
            AvatarUtil::pixmap(service->userId(), service->userName(), service->selfAvatar()));
    }
    else if (_isGroup)
    {
        // 群聊：每个"别人"都可能是不同的成员，用消息本身带的 fromId / fromName
        pChatItem->setUserName(msg.fromName.isEmpty() ? QString::number(msg.fromId) : msg.fromName);
        pChatItem->setUserIcon(AvatarUtil::pixmap(msg.fromId, msg.fromName));
    }
    else
    {
        // 单聊：只有一个"对方"，用 _friend
        const QString name = _friend.displayName().isEmpty() ? msg.fromName : _friend.displayName();
        pChatItem->setUserName(name);
        pChatItem->setUserIcon(
            AvatarUtil::pixmap(_friend.id, _friend.name, _friend.avatar));
    }

    // 目前只接文本消息，图片/文件等其它类型先按文本展示
    QWidget *pBubble = new TextBubble(role, msg.text);
    pChatItem->setWidget(pBubble);
    ui->chat_data_list->appendChatItem(pChatItem);
}

void ChatPage::on_send_btn_clicked()
{
    ChatService *service = ChatService::instance();
    if (!_isGroup && _friendId <= 0)
    {
        return;
    }
    if (_isGroup && _groupId <= 0)
    {
        return;
    }

    const QVector<MsgInfo> &msgList = ui->chat_edit->getMsgList();
    for (const MsgInfo &info : msgList)
    {
        // 图片/文件发送暂未接入协议，只发文本
        if (info.msgFlag != QStringLiteral("text") || info.content.isEmpty())
        {
            continue;
        }

        if (_isGroup)
        {
            service->sendGroupChat(_groupId, info.content);

            // 群聊：服务端会转发回自己，让 onChatMessage 统一渲染；这里也乐观渲染一份兜底
            ChatMessage msg;
            msg.fromId = service->userId();
            msg.groupId = _groupId;
            msg.fromName = service->userName();
            msg.text = info.content;
            msg.time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            msg.type = 0;
            appendMessage(msg);
            emit messageSent(msg);
        }
        else
        {
            service->sendOneChat(_friendId, info.content);

            // 单聊：服务端不回 ONE_CHAT_ACK，本地乐观渲染自己发的气泡
            ChatMessage msg;
            msg.fromId = service->userId();
            msg.toId = _friendId;
            msg.fromName = service->userName();
            msg.text = info.content;
            msg.time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            msg.type = 0;
            appendMessage(msg);
            emit messageSent(msg);
        }
    }
}
