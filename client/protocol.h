#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QJsonObject>
#include <QString>

// 与服务端 include/public.hpp 里的 EnMsgType 一一对应，改动时必须两边同步
namespace MsgType {
enum : int {
    LOGIN = 1,
    LOGIN_ACK = 2,
    REG = 3,
    REG_ACK = 4,
    ONE_CHAT = 5,
    ADD_FRIEND = 6,
    CREATE_GROUP = 7,
    ADD_GROUP = 8,
    GROUP_CHAT = 9,
    LOGINOUT = 10,
    LOGINOUT_ACK = 11,
    QUERY_HISTORY = 12,
    QUERY_HISTORY_ACK = 13,
    ONE_CHAT_ACK = 14,
    ADD_FRIEND_ACK = 15,
    GET_FRIEND_REQ = 16,
    GET_FRIEND_REQ_ACK = 17,
    HANDLE_FRIEND_REQ = 18,
    HANDLE_FRIEND_REQ_ACK = 19,
    GROUP_CHAT_ACK = 20,
    FRIEND_REQ_NOTIFY = 21,
    CREATE_GROUP_ACK = 22,
    ADD_GROUP_ACK = 23,

    UPDATE_REMARK = 24,
    UPDATE_REMARK_ACK = 25,

    // 好友申请被同意后服务端主动推给申请人：你的好友列表变了
    FRIEND_LIST_NOTIFY = 26,

    // 忘记密码 / 修改密码：按 userid 定位行更新 password
    CHANGE_PASS = 27,
    CHANGE_PASS_ACK = 28
};
}

// 每种请求的 JSON 构造，返回的对象可直接交给 TcpClient::sendFrame
namespace Protocol {

QJsonObject login(int userid, const QString &password);
QJsonObject reg(const QString &name, const QString &password);

QJsonObject oneChat(int myid, const QString &myname, int toid, const QString &text);
QJsonObject groupChat(int myid, const QString &myname, int groupid, const QString &text);

QJsonObject addFriend(int myid, int friendid, const QString &applyMsg = QString());
QJsonObject getFriendRequests(int myid);
QJsonObject handleFriendRequest(int myid, int reqid, bool accept);

QJsonObject createGroup(int myid, const QString &name, const QString &desc);
QJsonObject addGroup(int myid, int groupid);
QJsonObject updateRemark(int myid, int friendid, const QString &remark);
// 群备注：复用 UPDATE_REMARK 消息类型，把目标字段换成 groupid
QJsonObject updateGroupRemark(int myid, int groupid, const QString &remark);

// beforeId 传 0 表示取最新的一页，返回结果的 msgs 里带 id，用它当下一页的 beforeId
QJsonObject queryPrivateHistory(int myid, int friendid, qint64 beforeId = 0, int limit = 50);
QJsonObject queryGroupHistory(int myid, int groupid, qint64 beforeId = 0, int limit = 50);

QJsonObject logout(int myid);

// 忘记密码：按 userid 重置密码（非登录态也能调）
// confirmPassword 客户端本地已经校验过和 newPassword 一致，服务端再校验一次做防御
QJsonObject changePassword(int userid, const QString &newPassword, const QString &confirmPassword);

}

#endif // PROTOCOL_H
