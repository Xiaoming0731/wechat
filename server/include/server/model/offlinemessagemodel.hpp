#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel
{
public:
    // 只存投递引用：userid 待接收，mid 指向 message 表的自增 id
    // 消息内容不在这里存第二份，message 表是唯一真源
    void insert(int userid, long long mid);

    // 登录时取出待投递的消息 id（升序）
    vector<long long> query(int userid);

    // 投递成功后删除 userid 名下 mid <= maxMid 的记录，避免误删并发新写入的行
    void remove(int userid, long long maxMid);
};

#endif
