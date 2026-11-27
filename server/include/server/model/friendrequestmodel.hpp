#ifndef FRIENDREQUESTMODEL_H
#define FRIENDREQUESTMODEL_H

#include <string>
#include <utility>
#include <vector>
using namespace std;

// create 返回 pair：first=errno(0成功), second=reqid(仅成功时有效)
// errno 说明：
// 0 成功
// 2 不能添加自己
// 3 已经是好友
// 4 已有待处理的好友申请
// 5 对方用户不存在
// 6 申请不存在或已被处理
// 7 无权处理该申请
class FriendRequestModel
{
public:
    pair<int, int> create(int fromid, int toid, const string &applyMsg);

    // 返回待处理申请的 JSON 字符串数组
    vector<string> queryPending(int userid);

    // accept=true 同意，false 拒绝；同意时在事务里双向写入 friend
    // outFromId 非空时回填申请人 userid，供上层给申请人发系统消息
    int handle(int reqid, int userid, bool accept, int *outFromId = nullptr);
};

#endif
