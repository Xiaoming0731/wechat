#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Logging.h>
#include <string>

using namespace muduo;
using namespace muduo::net;

int main()
{
    EventLoop loop;
    InetAddress addr(2007);                       // 监听 2007 端口
    TcpServer server(&loop, addr, "EchoServer");

    server.setConnectionCallback(
        [](const TcpConnectionPtr& conn) {
            LOG_INFO << conn->peerAddress().toIpPort() << " -> "
                     << conn->localAddress().toIpPort() << " is "
                     << (conn->connected() ? "UP" : "DOWN");
        });

    server.setMessageCallback(
        [](const TcpConnectionPtr& conn,
           Buffer* buf,
           Timestamp /*time*/) {
            std::string msg = buf->retrieveAllAsString();
            conn->send(msg);                      // 原样回显
        });

    server.setThreadNum(2);
    server.start();
    loop.loop();
    return 0;
}
