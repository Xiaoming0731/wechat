#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include "chatservice.h"

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();

    // 让 ChatPage 知道"我在跟谁聊" —— 设置 title_lb 等
    void SetFriend(const FriendInfo &info);
    // 让 ChatPage 知道"我在哪个群聊" —— 设置标题、切换到群聊模式
    void SetGroup(const GroupInfo &info);

    // 追加一条消息气泡：在线接收、离线补发、历史加载、本地发送统一走这里
    void appendMessage(const ChatMessage &msg);

    void clearMessages();

signals:
    // 自己成功发出一条消息（供会话列表更新预览并置顶）
    void messageSent(const ChatMessage &msg);

    // 点设置按钮 → 弹 SetRemark；把当前上下文吐出去让 ChatDialog 处理
    // friendId>0 → 单聊改好友备注；groupId>0 → 群聊改群备注
    void settingClicked(int friendId, int groupId,
                        const QString &originalName,
                        const QString &currentRemark);

private slots:
    void on_send_btn_clicked();

private:
    Ui::ChatPage *ui;
    int _friendId;
    FriendInfo _friend;
    int _groupId = 0;
    bool _isGroup = false;
    GroupInfo _group;
};

#endif // CHATPAGE_H
