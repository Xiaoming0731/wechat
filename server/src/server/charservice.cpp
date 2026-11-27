#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include <vector>
#include <ctime>
using namespace std;
using namespace placeholders;

static string nowString()
{
    time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[32] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

// 把某个用户的群列表序列化成帧字符串数组：登录、建群、加群、改备注回包共用
static vector<string> buildGroupFrames(GroupModel &groupModel, int userid)
{
    vector<string> vec;
    vector<Group> groupvec = groupModel.queryGroup(userid);
    for (Group &group : groupvec)
    {
        json js;
        js["id"] = group.getId();
        js["groupname"] = group.getName();
        js["groupdesc"] = group.getDesc();
        js["remark"] = group.getRemark();

        vector<string> users;
        for (GroupUser &user : group.getUsers())
        {
            json ju;
            ju["id"] = user.getId();
            ju["name"] = user.getName();
            ju["state"] = user.getState();
            ju["role"] = user.getRole();
            users.push_back(ju.dump());
        }
        js["users"] = users;

        vec.push_back(js.dump());
    }
    return vec;
}

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
ChatService::ChatService()
{
    _msgHandlerMap.emplace(LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3));
    _msgHandlerMap.emplace(REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3));
    _msgHandlerMap.emplace(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3));
    _msgHandlerMap.emplace(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3));
    _msgHandlerMap.emplace(CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3));
    _msgHandlerMap.emplace(ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3));
    _msgHandlerMap.emplace(GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3));
    _msgHandlerMap.emplace(LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3));
    _msgHandlerMap.emplace(QUERY_HISTORY_MSG, std::bind(&ChatService::queryHistory, this, _1, _2, _3));
    _msgHandlerMap.emplace(GET_FRIEND_REQ_MSG, std::bind(&ChatService::getFriendReq, this, _1, _2, _3));
    _msgHandlerMap.emplace(HANDLE_FRIEND_REQ_MSG, std::bind(&ChatService::handleFriendReq, this, _1, _2, _3));
    _msgHandlerMap.emplace(UPDATE_REMARK_MSG, std::bind(&ChatService::updateRemark, this, _1, _2, _3));
    _msgHandlerMap.emplace(CHANGE_PASS_MSG, std::bind(&ChatService::changePass, this, _1, _2, _3));

    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

void ChatService::reset()
{
    _userModel.resetState();
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [&](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid  " << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 忘记密码：按 userid 重置密码，非登录态也能调
// 客户端本地已校验两次密码一致，这里再校验一次做防御
void ChatService::changePass(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);
    string pwd = js.value("password", string());
    string confirm = js.value("confirm_password", string());

    json response;
    response["msgid"] = CHANGE_PASS_MSG_ACK;

    if (userid <= 0 || pwd.empty() || confirm.empty())
    {
        response["errno"] = 1;
        response["errmsg"] = "参数错误";
        conn->send(response.dump());
        return;
    }

    if (pwd != confirm)
    {
        response["errno"] = 4;
        response["errmsg"] = "两次输入的密码不一致";
        conn->send(response.dump());
        return;
    }

    User user = _userModel.query(userid);
    if (user.getId() == -1)
    {
        response["errno"] = 2;
        response["errmsg"] = "用户不存在";
        conn->send(response.dump());
        return;
    }

    if (_userModel.updatePwd(userid, pwd))
    {
        response["errno"] = 0;
        response["errmsg"] = "密码修改成功";
    }
    else
    {
        response["errno"] = 3;
        response["errmsg"] = "密码修改失败";
    }
    conn->send(response.dump());
}

void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == -1)
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不存在!";
        conn->send(response.dump());
    }
    else
    {
        if (user.getPwd() == pwd)
        {
            if (user.getState() == "online")
            {
                json response;
                response["msgid"] = LOGIN_MSG_ACK;
                response["errno"] = 3;
                response["errmsg"] = "该用户已登录!";
                conn->send(response.dump());
            }
            else
            {
                {
                    lock_guard<mutex> lock(_connMutex);
                    _userConnMap.insert({id, conn});
                }

                _redis.subscribe(id);
                user.setState("online");
                _userModel.updateState(user);


                json response;
                response["msgid"] = LOGIN_MSG_ACK;
                response["errno"] = 0;
                response["id"] = user.getId();
                response["name"] = user.getName();
                response["avatar"] = user.getAvatar();

                // 离线消息只存了 message 表的 id，这里统一从 message 表取内容再下发
                vector<long long> offlineIds = _offlineMsgModel.query(id);
                if (!offlineIds.empty())
                {
                    vector<string> vec = _messageModel.queryFramesByIds(offlineIds);
                    if (!vec.empty())
                    {
                        response["offlinemsg"] = vec;
                    }
                    // 无论内容是否还在，引用都要清掉，避免离线队列无限堆积
                    _offlineMsgModel.remove(id, offlineIds.back());
                }

                vector<User> uservec = _friendModel.query(id);
                if (!uservec.empty())
                {
                    vector<string> vec2;
                    for (User &user : uservec)
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["avatar"] = user.getAvatar();
                        js["remark"] = user.getRemark();
                        vec2.push_back(js.dump());
                    }
                    response["friends"] = vec2;
                }

                // groups 里带 remark：每个用户看到的群备注可以不同
                vector<string> vec3 = buildGroupFrames(_groupModel, id);
                if (!vec3.empty())
                {
                    response["groups"] = vec3;
                }
                conn->send(response.dump());
            }
        }
        else
        {

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "密码输入错误!";
            conn->send(response.dump());
        }
    }
}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    _redis.unsubscribe(user.getId());
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"];
    int fromid = js["id"];
    int type = js.value("type", 0);
    string content = js.value("msg", string());

    // 非好友不允许单聊：直接回错误，既不落库也不转发
    if (!_friendModel.isFriend(fromid, toid))
    {
        json response;
        response["msgid"] = ONE_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "对方不是你的好友";
        response["to"] = toid;
        if (js.contains("seq"))
        {
            response["seq"] = js["seq"];
        }
        conn->send(response.dump());
        return;
    }

    // 先落库再转发，保证即使转发失败消息也不会丢
    long long mid = _messageModel.insert(fromid, toid, 0, type, content);

    // 把落库 id 带进帧里：客户端可用它去重，离线队列也靠它引用
    js["mid"] = mid;

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }
    User user=_userModel.query(toid);
    if(user.getState()=="online")
    {
        _redis.publish(toid,js.dump());
        return ;
    }
    _offlineMsgModel.insert(toid, mid);
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);
    int friendid = js.value("friendid", 0);
    string applyMsg = js.value("apply_msg", string());

    pair<int, int> res{1, 0};
    if (userid > 0 && friendid > 0)
    {
        // 以前是直接互加，现在改成写申请，等对方同意
        res = _friendRequestModel.create(userid, friendid, applyMsg);
    }
    int errnoCode = res.first;
    int reqid = res.second;

    json response;
    response["msgid"] = ADD_FRIEND_MSG_ACK;
    response["errno"] = errnoCode;
    response["friendid"] = friendid;

    switch (errnoCode)
    {
    case 0:  response["errmsg"] = "申请已发送"; break;
    case 2:  response["errmsg"] = "不能添加自己为好友"; break;
    case 3:  response["errmsg"] = "对方已经是你的好友"; break;
    case 4:  response["errmsg"] = "已发送过申请，请等待对方处理"; break;
    case 5:  response["errmsg"] = "用户不存在"; break;
    default: response["errmsg"] = "申请失败"; break;
    }
    conn->send(response.dump());

    // 申请成功后主动提醒对方：本机在线直接推，异地在线走 redis。
    // 不存离线消息，对方不在线时登录后查 friend_request 表一样能看到
    if (errnoCode == 0)
    {
        User applicant = _userModel.query(userid);

        json notify;
        notify["msgid"] = FRIEND_REQ_NOTIFY_MSG;
        notify["reqid"] = reqid;
        notify["from_id"] = userid;
        notify["from_name"] = applicant.getName();
        notify["apply_msg"] = applyMsg;
        notify["time"] = nowString();

        bool pushed = false;
        {
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnMap.find(friendid);
            if (it != _userConnMap.end())
            {
                it->second->send(notify.dump());
                pushed = true;
            }
        }
        if (!pushed)
        {
            User target = _userModel.query(friendid);
            if (target.getState() == "online")
            {
                _redis.publish(friendid, notify.dump());
            }
        }
    }
}

void ChatService::getFriendReq(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);

    json response;
    response["msgid"] = GET_FRIEND_REQ_MSG_ACK;

    if (userid <= 0)
    {
        response["errno"] = 1;
        response["errmsg"] = "参数错误";
        conn->send(response.dump());
        return;
    }

    response["errno"] = 0;
    response["requests"] = _friendRequestModel.queryPending(userid);
    conn->send(response.dump());
}

void ChatService::handleFriendReq(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);
    long long reqid = js.value("reqid", 0LL);
    bool accept = js.value("accept", false);

    int fromid = 0;
    int errnoCode = 1;
    if (userid > 0 && reqid > 0)
    {
        errnoCode = _friendRequestModel.handle((int)reqid, userid, accept, &fromid);
    }

    json response;
    response["msgid"] = HANDLE_FRIEND_REQ_MSG_ACK;
    response["errno"] = errnoCode;
    response["reqid"] = reqid;
    response["accept"] = accept;

    switch (errnoCode)
    {
    case 0:  response["errmsg"] = accept ? "已添加为好友" : "已拒绝该申请"; break;
    case 6:  response["errmsg"] = "申请不存在或已被处理"; break;
    case 7:  response["errmsg"] = "无权处理该申请"; break;
    default: response["errmsg"] = "处理失败"; break;
    }

    // 同意成功时顺便把最新好友列表带回去，客户端省一次额外请求
    if (errnoCode == 0 && accept)
    {
        vector<User> uservec = _friendModel.query(userid);
        vector<string> vec;
        for (User &u : uservec)
        {
            json js2;
            js2["id"] = u.getId();
            js2["name"] = u.getName();
            js2["state"] = u.getState();
            js2["avatar"] = u.getAvatar();
            js2["remark"] = u.getRemark();
            vec.push_back(js2.dump());
        }
        response["friends"] = vec;
    }

    conn->send(response.dump());

    // 同意成功后，申请人那边的好友列表也变了：主动推一份最新列表过去。
    // 不推的话申请人仍认为双方不是好友，点开会话也无法发送消息。
    if (errnoCode == 0 && accept && fromid > 0)
    {
        json notify;
        notify["msgid"] = FRIEND_LIST_NOTIFY_MSG;
        notify["friend_id"] = userid;   // 新增的好友

        vector<User> uservec = _friendModel.query(fromid);
        vector<string> vec;
        for (User &u : uservec)
        {
            json item;
            item["id"] = u.getId();
            item["name"] = u.getName();
            item["state"] = u.getState();
            item["avatar"] = u.getAvatar();
            item["remark"] = u.getRemark();
            vec.push_back(item.dump());
        }
        notify["friends"] = vec;

        bool pushed = false;
        {
            lock_guard<mutex> lock(_connMutex);
            auto it = _userConnMap.find(fromid);
            if (it != _userConnMap.end())
            {
                it->second->send(notify.dump());
                pushed = true;
            }
        }
        if (!pushed)
        {
            User target = _userModel.query(fromid);
            if (target.getState() == "online")
            {
                _redis.publish(fromid, notify.dump());
            }
        }
    }

    // 同意成功后，以同意方的身份给申请人发一条普通聊天消息：
    // 走 oneChat 的落库 + 在线转发 / 离线存储，申请人下次登录也能收到
    if (errnoCode == 0 && accept && fromid > 0)
    {
        User acceptor = _userModel.query(userid);

        json notice;
        notice["msgid"] = ONE_CHAT_MSG;
        notice["id"] = userid;
        notice["name"] = acceptor.getName();
        notice["to"] = fromid;
        notice["type"] = 0;
        notice["msg"] = "我已经同意了你的好友申请，现在开始聊天吧！";
        notice["time"] = nowString();

        oneChat(conn, notice, time);
    }
}
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    string name = js["groupname"];
    string desc = js["groupdesc"];

    json response;
    response["msgid"] = CREATE_GROUP_MSG_ACK;
    response["groupname"] = name;
    response["groupdesc"] = desc;

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
        response["errno"] = 0;
        response["errmsg"] = "群创建成功";
        response["groupid"] = group.getId();
        // 顺便回最新群列表，客户端不用再单独拉一次
        response["groups"] = buildGroupFrames(_groupModel, userid);
    }
    else
    {
        response["errno"] = 1;
        response["errmsg"] = "群创建失败";
        response["groupid"] = 0;
    }
    conn->send(response.dump());
}

void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupid"];

    json response;
    response["msgid"] = ADD_GROUP_MSG_ACK;
    response["groupid"] = groupid;

    if (!_groupModel.isGroupExist(groupid))
    {
        response["errno"] = 2;
        response["errmsg"] = "群不存在";
    }
    else if (!_groupModel.addGroup(userid, groupid, "normal"))
    {
        response["errno"] = 1;
        response["errmsg"] = "加群失败";
    }
    else
    {
        response["errno"] = 0;
        response["errmsg"] = "已加入群聊";
        // 加群成功后同步最新群列表
        response["groups"] = buildGroupFrames(_groupModel, userid);
    }
    conn->send(response.dump());
}

void ChatService::updateRemark(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);
    int groupid = js.value("groupid", 0);
    string remark = js.value("remark", string());

    // 群备注：复用 UPDATE_REMARK_MSG，只把目标从 friendid 换成 groupid
    if (groupid > 0)
    {
        json response;
        response["msgid"] = UPDATE_REMARK_MSG_ACK;
        response["groupid"] = groupid;

        if (!_groupModel.isMember(userid, groupid))
        {
            response["errno"] = 1;
            response["errmsg"] = "你不在该群聊中";
        }
        else if (_groupModel.updateGroupRemark(userid, groupid, remark))
        {
            response["errno"] = 0;
            response["errmsg"] = "群备注已更新";
            // 成功时顺便回最新群列表
            response["groups"] = buildGroupFrames(_groupModel, userid);
        }
        else
        {
            response["errno"] = 2;
            response["errmsg"] = "更新失败";
        }
        conn->send(response.dump());
        return;
    }

    int friendid = js.value("friendid", 0);

    json response;
    response["msgid"] = UPDATE_REMARK_MSG_ACK;
    response["friendid"] = friendid;

    if (!_friendModel.isFriend(userid, friendid))
    {
        response["errno"] = 1;
        response["errmsg"] = "不是好友";
    }
    else if (_friendModel.updateRemark(userid, friendid, remark))
    {
        response["errno"] = 0;
        response["errmsg"] = "备注已更新";
        // 成功时顺便回最新好友列表，客户端省一次刷新
        vector<User> uservec = _friendModel.query(userid);
        vector<string> vec;
        for (User &u : uservec)
        {
            json js2;
            js2["id"] = u.getId();
            js2["name"] = u.getName();
            js2["state"] = u.getState();
            js2["avatar"] = u.getAvatar();
            js2["remark"] = u.getRemark();
            vec.push_back(js2.dump());
        }
        response["friends"] = vec;
    }
    else
    {
        response["errno"] = 2;
        response["errmsg"] = "更新失败";
    }
    conn->send(response.dump());
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int groupid = js["groupid"];
    int type = js.value("type", 0);
    string content = js.value("msg", string());

    // 不是群成员不允许发群消息：落库和转发之前就拦掉
    if (!_groupModel.isMember(userid, groupid))
    {
        json response;
        response["msgid"] = GROUP_CHAT_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "你不在该群聊中";
        response["groupid"] = groupid;
        conn->send(response.dump());
        return;
    }

    // 群聊先落库，to_id 填 0
    long long mid = _messageModel.insert(userid, 0, groupid, type, content);
    js["mid"] = mid;

    vector<int> useridvec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridvec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user=_userModel.query(id);
            if(user.getState()=="online")
            {
                _redis.publish(id,js.dump());
            }
            else 
            {
                _offlineMsgModel.insert(id, mid);
            }
            
        }
    }
}

void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    _redis.unsubscribe(userid);

    User user;
    user.setId(userid);
    user.setState("offline");
    _userModel.updateState(user);

    json response;
    response["msgid"] = LOGINOUT_MSG_ACK;
    response["errno"] = 0;
    conn->send(response.dump());
}

void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    json js=json::parse(msg.c_str());
    // 帧里带的是 message 表 id，离线队列只存这个引用
    long long mid = js.value("mid", 0LL);
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(js.dump());
        return ;
    }
    _offlineMsgModel.insert(userid, mid);
}

void ChatService::queryHistory(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js.value("id", 0);
    int friendid = js.value("friendid", 0);
    int groupid = js.value("groupid", 0);
    long long beforeId = js.value("before_id", 0LL);
    int limit = js.value("limit", 50);

    if (limit <= 0)
    {
        limit = 50;
    }
    if (limit > 200)
    {
        limit = 200;
    }

    json response;
    response["msgid"] = QUERY_HISTORY_MSG_ACK;

    if (userid <= 0)
    {
        response["errno"] = 1;
        response["errmsg"] = "参数错误";
        conn->send(response.dump());
        return;
    }

    // friendid 和 groupid 都为 0：查询会话摘要（登录后恢复会话列表用）
    if (groupid <= 0 && friendid <= 0)
    {
        response["msgs"] = _messageModel.querySessions(userid, limit);
        response["errno"] = 0;
        conn->send(response.dump());
        return;
    }

    if (groupid > 0)
    {
        response["msgs"] = _messageModel.queryGroupHistory(groupid, beforeId, limit);
    }
    else
    {
        response["msgs"] = _messageModel.queryPrivateHistory(userid, friendid, beforeId, limit);
    }
    response["errno"] = 0;
    conn->send(response.dump());
}

/*
登录：
{"msgid":1,"id":1,"password":"123456"}
{"msgid":1,"id":2,"password":"333"}
{"msgid":1,"id":3,"password":"444"}

注册：
{"msgid":3,"name":"xiao liu","password":"333"}

发消息：
{"msgid":5,"id":1,"from":"xiao ming","to":2,"msg":"hello!"}
{"msgid":5,"id":1,"from":"xiao ming","to":2,"msg":"how are you!"}
{"msgid":5,"id":2,"from":"xiao liu","to":1,"msg":"how are you?"}
{"msgid":5,"id":2,"from":"xiao liu","to":1,"msg":"I'm ok,and you?"}

加好友：
{"msgid":6,"id":1,"friendid":2}
*/
