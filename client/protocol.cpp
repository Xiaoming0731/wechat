#include "protocol.h"

#include <QDateTime>

namespace {

// 服务端转发聊天消息时直接沿用客户端给的时间，格式要和数据库保持一致
QString nowString()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

}

namespace Protocol {

QJsonObject login(int userid, const QString &password)
{
    QJsonObject js;
    js["msgid"] = MsgType::LOGIN;
    js["id"] = userid;
    js["password"] = password;
    return js;
}

QJsonObject reg(const QString &name, const QString &password)
{
    QJsonObject js;
    js["msgid"] = MsgType::REG;
    js["name"] = name;
    js["password"] = password;
    return js;
}

QJsonObject oneChat(int myid, const QString &myname, int toid, const QString &text)
{
    QJsonObject js;
    js["msgid"] = MsgType::ONE_CHAT;
    js["id"] = myid;
    js["name"] = myname;
    js["to"] = toid;
    js["msg"] = text;
    js["type"] = 0;
    js["time"] = nowString();
    return js;
}

QJsonObject groupChat(int myid, const QString &myname, int groupid, const QString &text)
{
    QJsonObject js;
    js["msgid"] = MsgType::GROUP_CHAT;
    js["id"] = myid;
    js["name"] = myname;
    js["groupid"] = groupid;
    js["msg"] = text;
    js["type"] = 0;
    js["time"] = nowString();
    return js;
}

QJsonObject addFriend(int myid, int friendid, const QString &applyMsg)
{
    QJsonObject js;
    js["msgid"] = MsgType::ADD_FRIEND;
    js["id"] = myid;
    js["friendid"] = friendid;
    js["apply_msg"] = applyMsg;
    return js;
}

QJsonObject getFriendRequests(int myid)
{
    QJsonObject js;
    js["msgid"] = MsgType::GET_FRIEND_REQ;
    js["id"] = myid;
    return js;
}

QJsonObject handleFriendRequest(int myid, int reqid, bool accept)
{
    QJsonObject js;
    js["msgid"] = MsgType::HANDLE_FRIEND_REQ;
    js["id"] = myid;
    js["reqid"] = reqid;
    js["accept"] = accept;
    return js;
}

QJsonObject createGroup(int myid, const QString &name, const QString &desc)
{
    QJsonObject js;
    js["msgid"] = MsgType::CREATE_GROUP;
    js["id"] = myid;
    js["groupname"] = name;
    js["groupdesc"] = desc;
    return js;
}

QJsonObject addGroup(int myid, int groupid)
{
    QJsonObject js;
    js["msgid"] = MsgType::ADD_GROUP;
    js["id"] = myid;
    js["groupid"] = groupid;
    return js;
}

QJsonObject updateRemark(int myid, int friendid, const QString &remark)
{
    QJsonObject js;
    js["msgid"] = MsgType::UPDATE_REMARK;
    js["id"] = myid;
    js["friendid"] = friendid;
    js["remark"] = remark;
    return js;
}

QJsonObject updateGroupRemark(int myid, int groupid, const QString &remark)
{
    QJsonObject js;
    js["msgid"] = MsgType::UPDATE_REMARK;
    js["id"] = myid;
    js["groupid"] = groupid;
    js["remark"] = remark;
    return js;
}

QJsonObject queryPrivateHistory(int myid, int friendid, qint64 beforeId, int limit)
{
    QJsonObject js;
    js["msgid"] = MsgType::QUERY_HISTORY;
    js["id"] = myid;
    js["friendid"] = friendid;
    js["before_id"] = beforeId;
    js["limit"] = limit;
    return js;
}

QJsonObject queryGroupHistory(int myid, int groupid, qint64 beforeId, int limit)
{
    QJsonObject js;
    js["msgid"] = MsgType::QUERY_HISTORY;
    js["id"] = myid;
    js["groupid"] = groupid;
    js["before_id"] = beforeId;
    js["limit"] = limit;
    return js;
}

QJsonObject logout(int myid)
{
    QJsonObject js;
    js["msgid"] = MsgType::LOGINOUT;
    js["id"] = myid;
    return js;
}

QJsonObject changePassword(int userid, const QString &newPassword, const QString &confirmPassword)
{
    QJsonObject js;
    js["msgid"] = MsgType::CHANGE_PASS;
    js["id"] = userid;
    js["password"] = newPassword;
    js["confirm_password"] = confirmPassword;
    return js;
}

}
