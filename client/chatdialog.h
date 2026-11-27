#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"
#include "chatservice.h"   // FriendInfo

class AddFriendPage;
class AddGroupPage;
class CreateGroupPage;
class NewFriendPage;
class FriendPage;
class ChatUserWid;
class GroupList;
class SetRemark;
class QAction;

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();

signals:
    void switchLogin();

private slots:
    void onFriendClicked(int friendId);

private:
    // 侧边栏切页的唯一实现：stackedWidget 索引、_state、搜索栏显隐都在这里同步
    void switchPage(int id);
    // 进入和某个好友的聊天：确保 ChatUserList 有这个条目（没有就建、有就顶到最前），
    // 切到 ChatPage 并同步好友信息
    void openChatWith(const FriendInfo &f);
    // 进入和某个群的聊天：同上，但群聊用 GroupInfo + groupPixmap
    void openChatWithGroup(const GroupInfo &g);

    // 登录后恢复会话列表
    void reloadSessions();
    void onSessionsReady(const QVector<SessionInfo> &sessions);

    // 聊天消息：在线实时、历史回包、登录离线补发
    void onChatMessage(const ChatMessage &msg);
    void onHistoryReady(int friendid, const QVector<ChatMessage> &msgs);
    void onGroupHistoryReady(int groupid, const QVector<ChatMessage> &msgs);
    void onOfflineReady();
    void renderOfflineMessages(const QVector<ChatMessage> &msgs);

    // 会话列表：找条目 / 更新预览并置顶
    FriendInfo friendById(int id, const QString &fallbackName = QString()) const;
    GroupInfo groupById(int id, const QString &fallbackName = QString()) const;
    ChatUserWid *findSession(int friendId, int *row) const;
    ChatUserWid *findGroupSession(int groupId, int *row) const;
    void promoteSession(const FriendInfo &f, const QString &preview, const QString &time);
    void promoteGroupSession(const GroupInfo &g, const QString &preview, const QString &time);
    void updateSessionPreview(int friendId, const QString &preview, const QString &time);
    void updateGroupSessionPreview(int groupId, const QString &preview, const QString &time);
    // 清掉"没发过消息的临时会话"（离开该会话时调用，keepFriendId 为要保留的那个）
    void removeTempSessions(int keepFriendId);
    // 备注变更后，把会话列表里已有条目的显示名同步成最新（不重建条目、不动顺序）
    void refreshSessionNames();

    // 搜索：按关键词过滤好友+群，重建 search_list
    void refreshSearchResults(const QString &keyword);
    // 退出搜索：清空输入框、还原清除图标、清空结果列表（不发 textChanged）
    void resetSearch();

    void ShowSearch(bool bsearch=false);
    void updateSelfAvatar();
    void onAddFriendClicked();
    void onAddGroupClicked();
    void onCreateGroupClicked();
    void onNewFriendClicked();

    // 统一入口：弹出 SetRemark 弹窗并填入默认值；确认后调 ChatService
    void openSetRemark(int targetId, bool isGroup,
                       const QString &originalName,
                       const QString &currentRemark);

    Ui::ChatDialog *ui;
    ChatUIMode _mode;   // 当前实际显示的界面（搜索时为 SearchMode）
    ChatUIMode _state ;  // 用户选的主页面，不受搜索影响
    AddFriendPage *_addFriendPage;
    AddGroupPage *_addGroupPage;
    CreateGroupPage *_createGroupPage;
    NewFriendPage *_newFriendPage;
    FriendPage *_friendPage;  // 点好友才动态 new；切走时 delete，保持右侧空白
    SetRemark *_setRemarkPage;  // 设置备注弹窗，延迟创建
    QAction *_searchClearAction;  // search_edit 末尾的清除按钮，退出搜索时要还原图标
    int _currentFriendId;    // 0 表示当前没显示任何好友；用于 toggle 判断
    FriendInfo _activeChatFriend;  // 当前 ChatPage 在聊的好友（title_lb 显示对象）
    GroupInfo _activeChatGroup;    // 当前 ChatPage 在聊的群（title_lb 显示对象）；id=0 表示没开群
};

#endif // CHATDIALOG_H
