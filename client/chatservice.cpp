#include "chatservice.h"
#include "protocol.h"

#include <QJsonArray>
#include <QJsonDocument>

namespace {

// groups 是字符串数组，每个元素里再套一层 JSON（和 friends 同样的下发格式）
QVector<GroupInfo> groupsFromArray(const QJsonArray &arr)
{
    QVector<GroupInfo> list;
    for (const QJsonValue &value : arr)
    {
        const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
        GroupInfo info;
        info.id = obj.value("id").toInt();
        info.name = obj.value("groupname").toString();
        info.desc = obj.value("groupdesc").toString();
        info.remark = obj.value("remark").toString();
        info.avatar = obj.value("avatar").toString();
        info.ownerid = obj.value("ownerid").toInt();
        if (info.id > 0)
        {
            list.push_back(info);
        }
    }
    return list;
}

}

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService(QObject *parent)
    : QObject(parent),
      _client(new TcpClient(this)),
      _host("127.0.0.1"),
      _port(8010),   // nginx stream 集群入口，后端是 6001/6002
      _connecting(false),
      _userId(0),
      _applyUnread(false),
      _initApplyRed(false),
      _pendingHistoryFriendId(0),
      _pendingHistoryGroupId(0)
{
    connect(_client, &TcpClient::frameReady, this, &ChatService::onFrame);
    connect(_client, &TcpClient::connected, this, &ChatService::onConnected);
    connect(_client, &TcpClient::connectFailed, this, &ChatService::onConnectFailed);
}

QString ChatService::sessionKey(int friendId, int groupId)
{
    // 单聊：friendId>0 groupId=0；群聊：friendId=0 groupId>0
    // 用 '_' 分隔即可，不会歧义
    return QStringLiteral("%1_%2").arg(friendId).arg(groupId);
}

bool ChatService::isSessionUnread(int friendId, int groupId) const
{
    return _unreadSessions.contains(sessionKey(friendId, groupId));
}

void ChatService::markSessionUnread(int friendId, int groupId)
{
    if (friendId == 0 && groupId == 0) { return; }   // 无效会话
    const QString key = sessionKey(friendId, groupId);
    const bool wasEmpty = _unreadSessions.isEmpty();
    _unreadSessions.insert(key);
    emit sessionUnreadChanged(key, true);
    if (wasEmpty && !_unreadSessions.isEmpty())
    {
        emit chatSidebarRedChanged(true);
    }
}

void ChatService::markSessionRead(int friendId, int groupId)
{
    if (friendId == 0 && groupId == 0) { return; }
    const QString key = sessionKey(friendId, groupId);
    const bool contained = _unreadSessions.remove(key);
    if (contained)
    {
        emit sessionUnreadChanged(key, false);
        if (_unreadSessions.isEmpty())
        {
            emit chatSidebarRedChanged(false);
        }
    }
}

void ChatService::login(int userid, const QString &password)
{
    send(Protocol::login(userid, password));
}

void ChatService::reg(const QString &name, const QString &password)
{
    send(Protocol::reg(name, password));
}

void ChatService::changePassword(int userid, const QString &newPassword, const QString &confirmPassword)
{
    send(Protocol::changePassword(userid, newPassword, confirmPassword));
}

void ChatService::logout()
{
    if (_client->isConnected())
    {
        _client->sendFrame(Protocol::logout(_userId));
    }
    _userId = 0;
    _userName.clear();
    _avatar.clear();
    _friends.clear();
    _groups.clear();
    _applyUnread = false;
    _initApplyRed = false;
    _pendingHistoryFriendId = 0;
    _pendingHistoryGroupId = 0;
    _offlineMessages.clear();
    emit friendApplyRedChanged(false);
    _unreadSessions.clear();
    emit chatSidebarRedChanged(false);
}

void ChatService::sendOneChat(int toid, const QString &text)
{
    if (!isLoggedIn() || toid <= 0 || text.isEmpty())
    {
        return;
    }
    send(Protocol::oneChat(_userId, _userName, toid, text));
}

void ChatService::sendGroupChat(int groupid, const QString &text)
{
    if (!isLoggedIn() || groupid <= 0 || text.isEmpty())
    {
        return;
    }
    send(Protocol::groupChat(_userId, _userName, groupid, text));
}

void ChatService::queryHistory(int friendid, qint64 beforeId, int limit)
{
    if (!isLoggedIn() || friendid <= 0)
    {
        return;
    }
    _pendingHistoryFriendId = friendid;
    _pendingHistoryGroupId = 0;
    send(Protocol::queryPrivateHistory(_userId, friendid, beforeId, limit));
}

void ChatService::queryGroupHistory(int groupid, qint64 beforeId, int limit)
{
    if (!isLoggedIn() || groupid <= 0)
    {
        return;
    }
    _pendingHistoryGroupId = groupid;
    _pendingHistoryFriendId = 0;
    send(Protocol::queryGroupHistory(_userId, groupid, beforeId, limit));
}

void ChatService::querySessions(int limit)
{
    if (!isLoggedIn())
    {
        return;
    }
    // friendid/groupid 都为 0 时服务端返回会话摘要，复用 QUERY_HISTORY 不新增协议
    _pendingHistoryFriendId = 0;
    _pendingHistoryGroupId = 0;
    send(Protocol::queryPrivateHistory(_userId, 0, 0, limit));
}

QVector<ChatMessage> ChatService::takeOfflineMessages()
{
    const QVector<ChatMessage> list = _offlineMessages;
    _offlineMessages.clear();
    return list;
}

void ChatService::addFriend(int myid, int friendid, const QString &applyMsg)
{
    send(Protocol::addFriend(myid, friendid, applyMsg));
}

void ChatService::getFriendRequests(int myid)
{
    send(Protocol::getFriendRequests(myid));
}

void ChatService::handleFriendRequest(int myid, int reqid, bool accept)
{
    send(Protocol::handleFriendRequest(myid, reqid, accept));
}

void ChatService::addGroup(int groupid)
{
    send(Protocol::addGroup(_userId, groupid));
}

void ChatService::createGroup(const QString &name, const QString &desc)
{
    send(Protocol::createGroup(_userId, name, desc));
}

void ChatService::updateRemark(int friendid, const QString &remark)
{
    send(Protocol::updateRemark(_userId, friendid, remark));
}

void ChatService::updateGroupRemark(int groupid, const QString &remark)
{
    send(Protocol::updateGroupRemark(_userId, groupid, remark));
}

void ChatService::clearApplyUnread()
{
    if (!_applyUnread) { return; }
    _applyUnread = false;
    emit friendApplyRedChanged(false);
}

void ChatService::requestFriendApplyRedInit()
{
    if (!isLoggedIn()) { return; }
    _initApplyRed = true;
    send(Protocol::getFriendRequests(_userId));
}

void ChatService::send(const QJsonObject &frame)
{
    if (_client->isConnected())
    {
        _client->sendFrame(frame);
        return;
    }

    _outbox.push_back(frame);
    if (!_connecting)
    {
        _connecting = true;
        _client->connectTo(_host, _port);
    }
}

void ChatService::flushOutbox()
{
    const QVector<QJsonObject> frames = _outbox;
    _outbox.clear();
    for (const QJsonObject &frame : frames)
    {
        _client->sendFrame(frame);
    }
}

void ChatService::onConnected()
{
    _connecting = false;
    flushOutbox();
}

void ChatService::onConnectFailed(const QString &reason)
{
    _connecting = false;
    _outbox.clear();
    emit networkError(reason);
}

void ChatService::onFrame(const QJsonObject &frame)
{
    const int msgid = frame.value("msgid").toInt();
    qDebug() << "[ChatService::onFrame] msgid=" << msgid << "frame=" << frame;
    switch (msgid)
    {
    case MsgType::LOGIN_ACK:
        handleLoginAck(frame);
        break;
    case MsgType::REG_ACK:
        handleRegAck(frame);
        break;
    case MsgType::CHANGE_PASS_ACK: {
        const int err = frame.value("errno").toInt();
        if (err == 0)
        {
            emit changePassOk();
        }
        else
        {
            emit changePassFailed(frame.value("errmsg").toString());
        }
        break;
    }
    case MsgType::LOGINOUT_ACK:
        _userId = 0;
        _userName.clear();
        _avatar.clear();
        _friends.clear();
        _groups.clear();
        _applyUnread = false;
        _initApplyRed = false;
        _unreadSessions.clear();
        emit friendApplyRedChanged(false);
        emit chatSidebarRedChanged(false);
        break;
    case MsgType::ADD_FRIEND_ACK:
        emit addFriendResult(frame.value("errno").toInt(),
                             frame.value("errmsg").toString(),
                             frame.value("friendid").toInt());
        break;
    case MsgType::GET_FRIEND_REQ_ACK: {
        // requests 是字符串数组，每个元素里再套一层 JSON
        QVector<ApplyInfo> list;
        const QJsonArray arr = frame.value("requests").toArray();
        for (const QJsonValue &v : arr)
        {
            const QJsonObject obj = QJsonDocument::fromJson(v.toString().toUtf8()).object();
            ApplyInfo info;
            info.reqid = obj.value("reqid").toInt();
            info.fromId = obj.value("from_id").toInt();
            info.fromName = obj.value("from_name").toString();
            info.applyMsg = obj.value("apply_msg").toString();
            info.time = obj.value("time").toString();
            info.avatar = obj.value("avatar").toString();
            if (info.reqid > 0)
            {
                list.push_back(info);
            }
        }
        emit friendRequestsReady(list);

        // 只在登录初始化阶段用这次回包同步红点
        // NewFriendPage 主动拉列表不应该重置红点（点一次就消的语义才对）
        if (_initApplyRed)
        {
            _initApplyRed = false;
            _applyUnread = !list.isEmpty();
            emit friendApplyRedChanged(_applyUnread);
        }
        break;
    }
    case MsgType::HANDLE_FRIEND_REQ_ACK: {
        const int reqid = frame.value("reqid").toInt();
        const bool success = frame.value("errno").toInt() == 0;
        qDebug() << "[HANDLE_FRIEND_REQ_ACK] reqid=" << reqid << "success=" << success
                 << "has_friends=" << frame.contains("friends");
        // 同意成功后，服务端会顺便把最新好友列表一起带回来
        if (success && frame.contains("friends"))
        {
            _friends.clear();
            const QJsonArray arr = frame.value("friends").toArray();
            qDebug() << "[HANDLE_FRIEND_REQ_ACK] friends count=" << arr.size();
            for (const QJsonValue &value : arr)
            {
                const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
                FriendInfo info;
                info.id = obj.value("id").toInt();
                info.name = obj.value("name").toString();
                info.remark = obj.value("remark").toString();
                info.avatar = obj.value("avatar").toString();
                info.online = obj.value("state").toString() == QStringLiteral("online");
                qDebug() << "  friend:" << info.id << info.name << "remark=" << info.remark;
                if (info.id > 0)
                {
                    _friends.push_back(info);
                }
            }
            qDebug() << "[HANDLE_FRIEND_REQ_ACK] emitting friendListReady";
            emit friendListReady();
        }
        emit handleRequestResult(reqid, success);
        break;
    }
    case MsgType::FRIEND_REQ_NOTIFY:
        emit friendRequestNotify(frame.value("reqid").toInt(),
                                 frame.value("from_id").toInt(),
                                 frame.value("from_name").toString(),
                                 frame.value("apply_msg").toString());
        _applyUnread = true;
        emit friendApplyRedChanged(true);
        break;
    case MsgType::FRIEND_LIST_NOTIFY: {
        // 好友申请被同意：服务端主动推来最新的好友列表，整体替换
        _friends.clear();
        const QJsonArray arr = frame.value("friends").toArray();
        for (const QJsonValue &value : arr)
        {
            const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
            FriendInfo info;
            info.id = obj.value("id").toInt();
            info.name = obj.value("name").toString();
            info.remark = obj.value("remark").toString();
            info.avatar = obj.value("avatar").toString();
            info.online = obj.value("state").toString() == QStringLiteral("online");
            if (info.id > 0)
            {
                _friends.push_back(info);
            }
        }
        emit friendListReady();
        break;
    }
    case MsgType::ADD_GROUP_ACK:
        // 加群成功时服务端会带回最新群列表
        if (frame.value("errno").toInt() == 0 && frame.contains("groups"))
        {
            _groups = groupsFromArray(frame.value("groups").toArray());
            emit groupListReady();
        }
        emit addGroupResult(frame.value("errno").toInt(),
                            frame.value("errmsg").toString(),
                            frame.value("groupid").toInt());
        break;
    case MsgType::CREATE_GROUP_ACK:
        // 建群成功时服务端会带回最新群列表
        if (frame.value("errno").toInt() == 0 && frame.contains("groups"))
        {
            _groups = groupsFromArray(frame.value("groups").toArray());
            emit groupListReady();
        }
        emit createGroupResult(frame.value("errno").toInt(),
                               frame.value("errmsg").toString(),
                               frame.value("groupid").toInt());
        break;
    case MsgType::UPDATE_REMARK_ACK: {
        const bool success = frame.value("errno").toInt() == 0;

        // 群备注：复用同一个 ACK，靠 groupid 字段区分
        if (frame.contains("groupid"))
        {
            const int groupid = frame.value("groupid").toInt();
            if (success && frame.contains("groups"))
            {
                _groups = groupsFromArray(frame.value("groups").toArray());
                emit groupListReady();
            }
            emit updateGroupRemarkResult(frame.value("errno").toInt(),
                                         frame.value("errmsg").toString(),
                                         groupid);
            break;
        }

        const int friendid = frame.value("friendid").toInt();
        if (success && frame.contains("friends"))
        {
            _friends.clear();
            const QJsonArray arr = frame.value("friends").toArray();
            for (const QJsonValue &value : arr)
            {
                const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
                FriendInfo info;
                info.id = obj.value("id").toInt();
                info.name = obj.value("name").toString();
                info.remark = obj.value("remark").toString();
                info.avatar = obj.value("avatar").toString();
                info.online = obj.value("state").toString() == QStringLiteral("online");
                if (info.id > 0)
                {
                    _friends.push_back(info);
                }
            }
            emit friendListReady();
        }
        emit updateRemarkResult(frame.value("errno").toInt(),
                                 frame.value("errmsg").toString(),
                                 friendid);
        break;
    }
    case MsgType::ONE_CHAT: {
        // 在线转发来的单聊消息（服务端已落库，帧里带 mid）
        const ChatMessage msg = messageFromFrame(frame);
        emit chatMessageReceived(msg);
        // 不是自己发的 → 对端会话置未读（单聊 fromId 是对方，friendId = fromId）
        if (msg.fromId != _userId)
        {
            markSessionUnread(msg.fromId, 0);
        }
        break;
    }
    case MsgType::GROUP_CHAT: {
        // 群聊：服务端会把群成员（含自己）全部转发一份
        const ChatMessage msg = messageFromFrame(frame);
        emit chatMessageReceived(msg);
        // 不是自己发的 → 该群会话置未读
        if (msg.fromId != _userId && msg.groupId > 0)
        {
            markSessionUnread(0, msg.groupId);
        }
        break;
    }
    case MsgType::QUERY_HISTORY_ACK: {
        const QJsonArray arr = frame.value("msgs").toArray();
        const int friendid = _pendingHistoryFriendId;
        const int groupid = _pendingHistoryGroupId;
        _pendingHistoryFriendId = 0;
        _pendingHistoryGroupId = 0;

        if (friendid == 0 && groupid == 0)
        {
            // 会话摘要：每条只带好友 id / 最后一条消息
            QVector<SessionInfo> sessions;
            for (const QJsonValue &v : arr)
            {
                const QJsonObject obj = QJsonDocument::fromJson(v.toString().toUtf8()).object();
                if (obj.isEmpty())
                {
                    continue;
                }
                SessionInfo s;
                s.friendId = (obj.value("groupid").toInt() == 0) ? obj.value("id").toInt() : 0;
                s.groupId = obj.value("groupid").toInt();
                s.name = obj.value("name").toString();
                s.lastMsg = obj.value("msg").toString();
                s.time = obj.value("time").toString();
                if (s.friendId > 0 || s.groupId > 0)
                {
                    sessions.push_back(s);
                }
            }
            emit sessionsReady(sessions);
            break;
        }

        QVector<ChatMessage> list;
        for (const QJsonValue &v : arr)
        {
            const QJsonObject obj = QJsonDocument::fromJson(v.toString().toUtf8()).object();
            if (!obj.isEmpty())
            {
                list.push_back(messageFromFrame(obj));
            }
        }

        if (groupid > 0)
        {
            emit groupHistoryReady(groupid, list);
        }
        else
        {
            emit chatHistoryReady(friendid, list);
        }
        break;
    }
    default:
        // 好友、聊天等消息留到后续接入时再处理
        break;
    }
}

ChatMessage ChatService::messageFromFrame(const QJsonObject &js)
{
    ChatMessage msg;
    msg.mid = js.value("mid").toVariant().toLongLong();
    msg.fromId = js.value("id").toInt();
    msg.toId = js.value("to").toInt();
    msg.groupId = js.value("groupid").toInt();
    msg.fromName = js.value("name").toString();
    msg.text = js.value("msg").toString();
    msg.time = js.value("time").toString();
    msg.type = js.value("type").toInt();
    return msg;
}

void ChatService::handleLoginAck(const QJsonObject &js)
{
    if (js.value("errno").toInt() != 0)
    {
        emit loginFailed(js.value("errmsg").toString(QStringLiteral("登录失败")));
        return;
    }

    _userId = js.value("id").toInt();
    _userName = js.value("name").toString();
    _avatar = js.value("avatar").toString();

    // friends 是字符串数组，每个元素里再套一层 JSON
    _friends.clear();
    const QJsonArray arr = js.value("friends").toArray();
    for (const QJsonValue &value : arr)
    {
        const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
        FriendInfo info;
        info.id = obj.value("id").toInt();
        info.name = obj.value("name").toString();
        info.remark = obj.value("remark").toString();
        info.avatar = obj.value("avatar").toString();
        info.online = obj.value("state").toString() == QStringLiteral("online");
        if (info.id > 0)
        {
            _friends.push_back(info);
        }
    }

    // groups 结构和 friends 一样，元素里带 id/groupname/groupdesc/remark/users
    _groups = groupsFromArray(js.value("groups").toArray());

    // 离线消息：服务端只存了 message 表的 id，登录时已重建为完整帧
    _offlineMessages.clear();
    const QJsonArray offlineArr = js.value("offlinemsg").toArray();
    for (const QJsonValue &value : offlineArr)
    {
        const QJsonObject obj = QJsonDocument::fromJson(value.toString().toUtf8()).object();
        if (!obj.isEmpty())
        {
            const ChatMessage msg = messageFromFrame(obj);
            _offlineMessages.push_back(msg);
            // 离线未读消息也要记入未读集合：非自己发的 → 对应会话置未读
            // （此刻 ChatDialog 可能还没创建，红点会在它构造/填充列表时从真源恢复）
            if (msg.fromId != _userId)
            {
                if (msg.groupId > 0)
                {
                    markSessionUnread(0, msg.groupId);
                }
                else
                {
                    markSessionUnread(msg.fromId, 0);
                }
            }
        }
    }
    if (!_offlineMessages.isEmpty())
    {
        // ChatDialog 可能在登录成功之后才创建（重新登录），所以先发信号，
        // 由它自己决定是当场渲染还是从 takeOfflineMessages() 里取
        emit offlineMessagesReady();
    }

    emit loginOk(_userId, _userName);
    emit friendListReady();
    emit groupListReady();

    // 登录后立刻拉一次申请列表初始化红点
    requestFriendApplyRedInit();
}

void ChatService::handleRegAck(const QJsonObject &js)
{
    if (js.value("errno").toInt() != 0)
    {
        emit regFailed(js.value("errmsg").toString(QStringLiteral("注册失败")));
        return;
    }

    emit regOk(js.value("id").toInt());
}
