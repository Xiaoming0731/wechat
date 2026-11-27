#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QSet>

#include "tcpclient.h"

struct FriendInfo
{
    int id = 0;
    QString name;
    QString remark;     // 用户对该好友的备注，为空时显示 name
    QString avatar;
    bool online = false;
    // 显示名：有备注用备注，没备注用原昵称
    QString displayName() const { return remark.isEmpty() ? name : remark; }
};

struct GroupInfo
{
    int id=0;
    QString name;
    QString desc;
    QString avatar;
    int ownerid=0;
    QString remark;     // 当前用户对该群的个人备注，存 groupuser.remark

    // 显示名：有备注用备注，没备注用群名
    QString displayName() const { return remark.isEmpty() ? name : remark; }
};

// 好友申请条目（服务端 friend_request 表的一条记录）
struct ApplyInfo
{
    int reqid = 0;          // friend_request 表自增 id
    int fromId = 0;         // 申请人 uid
    QString fromName;       // 申请人昵称
    QString applyMsg;       // 申请附言
    QString time;           // 创建时间
    QString avatar;         // 申请人头像
};

// 一条聊天消息：在线实时、历史查询、离线补发统一用这个结构
struct ChatMessage
{
    qint64 mid = 0;     // message 表自增 id
    int fromId = 0;
    int toId = 0;
    int groupId = 0;    // >0 表示群聊消息
    QString fromName;
    QString text;
    QString time;
    int type = 0;       // 0 文本
};

// 会话摘要：某个好友或某个群的最后一条消息，用来在登录后恢复会话列表
struct SessionInfo
{
    int friendId = 0;     // 单聊时是对方 id，群聊时为 0
    int groupId = 0;      // 群聊时 >0，单聊时为 0
    QString name;
    QString lastMsg;
    QString time;
};

// 业务层单例：持有 TcpClient，把协议收发翻译成 Qt 信号给各页面用
class ChatService : public QObject
{
    Q_OBJECT

public:
    static ChatService *instance();

    void login(int userid, const QString &password);
    void reg(const QString &name, const QString &password);
    void logout();

    // 忘记密码/修改密码：按 userid 重置，非登录态也能调
    void changePassword(int userid, const QString &newPassword, const QString &confirmPassword);

    // 单聊：发送一条文本消息
    void sendOneChat(int toid, const QString &text);
    // 群聊：发送一条文本消息
    void sendGroupChat(int groupid, const QString &text);
    // 拉取与某好友的历史消息（beforeId 传 0 取最新一页），结果通过 chatHistoryReady 返回
    void queryHistory(int friendid, qint64 beforeId = 0, int limit = 50);
    // 拉取某个群的历史消息，结果通过 groupHistoryReady 返回
    void queryGroupHistory(int groupid, qint64 beforeId = 0, int limit = 50);
    // 拉取会话摘要列表（登录后恢复会话列表），结果通过 sessionsReady 返回
    void querySessions(int limit = 100);
    // 登录 ACK 里带的离线消息：取走并清空本地缓存
    QVector<ChatMessage> takeOfflineMessages();

    // 好友申请相关
    void addFriend(int myid, int friendid, const QString &applyMsg = QString());
    void getFriendRequests(int myid);
    void handleFriendRequest(int myid, int reqid, bool accept);

    // 加群
    void addGroup(int groupid);

    // 创建群
    void createGroup(const QString &name, const QString &desc);

    // 更新好友备注
    void updateRemark(int friendid, const QString &remark);
    // 更新群备注（只影响自己看到的群名）
    void updateGroupRemark(int groupid, const QString &remark);

    int userId() const { return _userId; }
    QString userName() const { return _userName; }
    QString selfAvatar() const { return _avatar; }
    QVector<FriendInfo> friends() const { return _friends; }
    QVector<GroupInfo> groups() const { return _groups; }
    bool isLoggedIn() const { return _userId > 0; }
    bool hasApplyUnread() const { return _applyUnread; }

    // 点「新的朋友」时调用，两个红点一起熄灭
    void clearApplyUnread();

    // 已在线但红点状态还没初始化时调用（比如 ChatDialog 切页后重建）
    // 内部会标记 init，下次 GET_FRIEND_REQ_ACK 回来会同步红点
    void requestFriendApplyRedInit();

    // ============== 会话未读红点 ==============
    // 单一真源：只有 ChatService 允许改动 _unreadSessions，UI 层只读/调 API
    static QString sessionKey(int friendId, int groupId);
    bool hasChatUnread() const { return !_unreadSessions.isEmpty(); }
    bool isSessionUnread(int friendId, int groupId) const;
    // 收到别人发来的消息时调；置未读 + emit sessionUnreadChanged + 如有必要 emit chatSidebarRedChanged
    void markSessionUnread(int friendId, int groupId);
    // 用户点击进入会话时调；清未读 + emit sessionUnreadChanged + 如有必要 emit chatSidebarRedChanged
    void markSessionRead(int friendId, int groupId);

signals:
    void loginOk(int userid, const QString &name);
    void loginFailed(const QString &errmsg);
    void regOk(int userid);
    void regFailed(const QString &errmsg);
    void changePassOk();
    void changePassFailed(const QString &errmsg);
    void friendListReady();
    void groupListReady();
    void networkError(const QString &errmsg);

    // 好友申请
    void addFriendResult(int err, const QString &errmsg, int friendId);
    void friendRequestsReady(const QVector<ApplyInfo> &list);
    void handleRequestResult(int reqid, bool success);
    void friendRequestNotify(int reqid, int fromId, const QString &fromName, const QString &applyMsg);
    // 红点变化信号：UI 全都只听这一条
    void friendApplyRedChanged(bool hasUnread);

    // 会话未读红点：某个会话的红点变化（单会话粒度）
    void sessionUnreadChanged(const QString &sessionKey, bool hasUnread);
    // 侧边栏聊天红点：任一会话有未读就 true，全清才 false
    void chatSidebarRedChanged(bool hasAnyUnread);

    // 加群
    void addGroupResult(int err, const QString &errmsg, int groupid);

    // 创建群
    void createGroupResult(int err, const QString &errmsg, int groupid);

    // 更新备注
    void updateRemarkResult(int err, const QString &errmsg, int friendid);
    void updateGroupRemarkResult(int err, const QString &errmsg, int groupid);

    // 聊天
    void chatMessageReceived(const ChatMessage &msg);                  // 在线实时收到
    void chatHistoryReady(int friendid, const QVector<ChatMessage> &msgs);
    void groupHistoryReady(int groupid, const QVector<ChatMessage> &msgs);
    void sessionsReady(const QVector<SessionInfo> &sessions);          // 会话摘要列表
    void offlineMessagesReady();                                       // 登录下发的离线消息已就绪

private slots:
    void onFrame(const QJsonObject &frame);
    void onConnected();
    void onConnectFailed(const QString &reason);

private:
    explicit ChatService(QObject *parent = nullptr);

    // 未连接时先排队，连上后统一发出去
    void send(const QJsonObject &frame);
    void flushOutbox();

    void handleLoginAck(const QJsonObject &js);
    void handleRegAck(const QJsonObject &js);
    // 把服务端帧（在线转发 / 历史 / 离线）转成 ChatMessage
    static ChatMessage messageFromFrame(const QJsonObject &js);

    TcpClient *_client;
    QString _host;
    quint16 _port;
    QVector<QJsonObject> _outbox;
    bool _connecting;

    int _userId;
    QString _userName;
    QString _avatar;
    QVector<FriendInfo> _friends;
    QVector<GroupInfo> _groups;
    bool _applyUnread;   // 好友申请红点：有未查看的申请则 true
    bool _initApplyRed;  // 登录后第一次 GET_FRIEND_REQ_ACK 用来初始化红点，处理后清 false
    // 会话未读红点：key 为 sessionKey(friendId, groupId)
    QSet<QString> _unreadSessions;

    int _pendingHistoryFriendId;          // 最近一次历史查询的好友 id（ACK 里不带该字段）
    int _pendingHistoryGroupId;           // 最近一次历史查询的群 id，>0 表示这次是群历史
    QVector<ChatMessage> _offlineMessages; // 登录 ACK 下发的离线消息，等 ChatDialog 取走
};

#endif // CHATSERVICE_H
