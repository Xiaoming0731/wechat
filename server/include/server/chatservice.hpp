#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include "json.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include "friendrequestmodel.hpp"
#include "groupmodel.hpp"
#include "messagemodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class ChatService
{
public:
    static ChatService* instance();
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    MsgHandler getHandler(int msgid);
    void clientCloseException(const TcpConnectionPtr &conn);
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void getFriendReq(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void handleFriendReq(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void updateRemark(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void changePass(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void queryHistory(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void reset();
    void handleRedisSubscribeMessage(int user,string msg);

private:
    ChatService();
    unordered_map<int,MsgHandler> _msgHandlerMap;
    UserModel _userModel;
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    mutex _connMutex; 
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    FriendRequestModel _friendRequestModel;
    GroupModel _groupModel; 
    MessageModel _messageModel;
    Redis _redis;
};

#endif
