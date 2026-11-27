#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
#include <stdlib.h>
using namespace std;

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc, char *argv[])
{
    signal(SIGINT, resetHandler);
    EventLoop loop;

    // 端口可通过命令行参数指定，便于同一台机器跑多个集群实例
    int port = 6000;
    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    // 监听 0.0.0.0，容器内才能被 nginx / 宿主机访问
    InetAddress addr("0.0.0.0", port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}