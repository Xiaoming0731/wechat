#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"
#include "muduo/base/Logging.h"
#include <algorithm>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json=nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 客户端每条 JSON 都以 '\0' 结尾。按 '\0' 拆包：
    // 一次 read 里可能有半包（等下次读）或多条消息（循环处理）
    for (;;)
    {
        const char *data = buffer->peek();
        const char *dataEnd = data + buffer->readableBytes();
        const char *frameEnd = std::find(data, dataEnd, '\0');
        if (frameEnd == dataEnd)
        {
            break; // 还没读到结尾的 '\0'，剩余的留给下一次读
        }

        string buf(data, frameEnd - data);
        buffer->retrieve(frameEnd - data + 1); // 连同 '\0' 一起消费掉

        if (buf.empty())
        {
            continue;
        }

        // 单条消息的异常只丢弃这一条，绝不能让异常抛到 muduo IO 线程外（会 abort 整个进程）
        try
        {
            json js = json::parse(buf);
            auto msghandler = ChatService::instance()->getHandler(js.at("msgid").get<int>());
            msghandler(conn, js, time);
        }
        catch (const exception &e)
        {
            LOG_ERROR << "handle message failed: " << e.what() << ", raw: " << buf;
        }
    }
}