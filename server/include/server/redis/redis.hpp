#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <mutex>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();
    bool connect();
    bool publish(int channel,string message);
    bool subscribe(int channel);
    bool unsubscribe(int channel);
    void observer_channel_message();
    void init_notify_handler(function<void(int,string)>fn);

private:
    redisContext* _publish_context;
    redisContext* _subscribe_context;
    // hiredis 的 redisContext 不是线程安全的，
    // 多个 IO 线程同时写同一连接会破坏堆内存，所有写操作串行化
    mutex _write_mutex;
    function<void(int,string)> _notify_message_handler;
};

#endif