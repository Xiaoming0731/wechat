#include "redis.hpp"
#include <iostream>
#include <cstdlib>
using namespace std;

// 默认值兼容本地开发，容器部署时通过环境变量覆盖
static string redisHost()
{
    const char *v = getenv("REDIS_HOST");
    return (v != nullptr && *v != '\0') ? string(v) : string("127.0.0.1");
}

static int redisPort()
{
    const char *v = getenv("REDIS_PORT");
    return (v != nullptr && *v != '\0') ? atoi(v) : 6379;
}

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}
bool Redis::connect()
{
    const string host = redisHost();
    const int port = redisPort();
    _publish_context = redisConnect(host.c_str(), port);
    if (_publish_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }
    _subscribe_context = redisConnect(host.c_str(), port);
    if (_subscribe_context == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis_server success!" << endl;
    return true;
}
bool Redis::publish(int channel, string message)
{
    lock_guard<mutex> lock(_write_mutex);
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
bool Redis::subscribe(int channel)
{
    lock_guard<mutex> lock(_write_mutex);
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}
bool Redis::unsubscribe(int channel)
{
    lock_guard<mutex> lock(_write_mutex);
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str) , reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler = fn;
}