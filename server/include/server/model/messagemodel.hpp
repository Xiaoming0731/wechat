#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <string>
#include <vector>
using namespace std;

class MessageModel
{
public:
    // 落库一条消息：groupid 为 0 表示单聊，此时 toid 为接收者
    // 返回该消息的自增 id（离线投递引用它），失败返回 0
    long long insert(int fromid, int toid, int groupid, int type, const string &content);

    // 单聊历史：beforeId 为游标（拉更早消息时传当前最老一条的 id，0 表示从最新开始）
    // 返回按时间升序排列的 JSON 字符串数组，字段与 ONE_CHAT_MSG 一致
    vector<string> queryPrivateHistory(int userid, int friendid, long long beforeId, int limit);

    // 群聊历史
    vector<string> queryGroupHistory(int groupid, long long beforeId, int limit);

    // 按消息 id 批量取消息，拼成可直接下发的帧（离线消息补发用）
    vector<string> queryFramesByIds(const vector<long long> &ids);

    // 会话摘要：每个聊过天的好友只取最后一条消息，按最新在前（登录后恢复会话列表用）
    vector<string> querySessions(int userid, int limit);
};

#endif
