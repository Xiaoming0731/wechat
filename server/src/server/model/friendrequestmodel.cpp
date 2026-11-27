#include "friendrequestmodel.hpp"
#include "db.h"
#include "json.hpp"
#include <cstdio>
#include <cstdlib>
using json = nlohmann::json;

pair<int, int> FriendRequestModel::create(int fromid, int toid, const string &applyMsg)
{
    if (fromid <= 0 || toid <= 0)
    {
        return {5, 0}; // 参数非法，按"对方不存在"处理
    }
    if (fromid == toid)
    {
        return {2, 0};
    }

    MySQL mysql;
    if (!mysql.connect())
    {
        return {5, 0};
    }

    char sql[2048] = {0};

    // 对方是否存在
    snprintf(sql, sizeof(sql), "select id from user where id=%d", toid);
    MYSQL_RES *res = mysql.query(sql);
    if (res == nullptr)
    {
        return {5, 0};
    }
    bool targetExists = (mysql_fetch_row(res) != nullptr);
    mysql_free_result(res);
    if (!targetExists)
    {
        return {5, 0};
    }

    // 已经是好友
    snprintf(sql, sizeof(sql), "select 1 from friend where userid=%d and friendid=%d limit 1", fromid, toid);
    res = mysql.query(sql);
    if (res == nullptr)
    {
        return {5, 0};
    }
    bool alreadyFriend = (mysql_fetch_row(res) != nullptr);
    mysql_free_result(res);
    if (alreadyFriend)
    {
        return {3, 0};
    }

    // 已有待处理申请
    snprintf(sql, sizeof(sql),
             "select 1 from friend_request where from_id=%d and to_id=%d and status=0 limit 1", fromid, toid);
    res = mysql.query(sql);
    if (res == nullptr)
    {
        return {5, 0};
    }
    bool pending = (mysql_fetch_row(res) != nullptr);
    mysql_free_result(res);
    if (pending)
    {
        return {4, 0};
    }

    // 写入申请；被拒绝过的那行靠唯一键重新激活
    string escaped = mysql.escape(applyMsg);
    string insertSql = "insert into friend_request(from_id,to_id,apply_msg,status) values(" +
                       to_string(fromid) + "," + to_string(toid) + ",'" + escaped + "',0) "
                       "on duplicate key update status=0, apply_msg='" + escaped + "', "
                       "created_at=now(), handled_at=null";

    if (!mysql.update(insertSql))
    {
        return {5, 0};
    }

    // 拿刚写入的 reqid（用 select 查，兼容 on duplicate key update 的场景）
    snprintf(sql, sizeof(sql),
             "select id from friend_request where from_id=%d and to_id=%d order by id desc limit 1",
             fromid, toid);
    res = mysql.query(sql);
    int reqid = 0;
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr && row[0] != nullptr)
        {
            reqid = atoi(row[0]);
        }
        mysql_free_result(res);
    }

    return {0, reqid};
}

vector<string> FriendRequestModel::queryPending(int userid)
{
    vector<string> vec;

    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "select r.id,r.from_id,u.name,u.avatar,r.apply_msg,"
             "date_format(r.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') "
             "from friend_request r join user u on u.id=r.from_id "
             "where r.to_id=%d and r.status=0 order by r.id desc",
             userid);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                json item;
                item["reqid"] = atoll(row[0]);
                item["from_id"] = atoi(row[1]);
                item["from_name"] = row[2] ? row[2] : "";
                item["avatar"] = row[3] ? row[3] : "";
                item["apply_msg"] = row[4] ? row[4] : "";
                item["time"] = row[5] ? row[5] : "";
                vec.push_back(item.dump());
            }
            mysql_free_result(res);
        }
    }
    return vec;
}

int FriendRequestModel::handle(int reqid, int userid, bool accept, int *outFromId)
{
    if (reqid <= 0 || userid <= 0)
    {
        return 6;
    }

    MySQL mysql;
    if (!mysql.connect())
    {
        return 6;
    }

    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select from_id,to_id,status from friend_request where id=%lld", (long long)reqid);

    MYSQL_RES *res = mysql.query(sql);
    if (res == nullptr)
    {
        return 6;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row == nullptr)
    {
        mysql_free_result(res);
        return 6;
    }
    int fromid = atoi(row[0]);
    int toid = atoi(row[1]);
    int status = atoi(row[2]);
    mysql_free_result(res);

    if (status != 0)
    {
        return 6; // 已被处理
    }
    if (toid != userid)
    {
        return 7; // 不是自己的申请
    }
    if (outFromId != nullptr)
    {
        *outFromId = fromid;
    }

    mysql.update("start transaction");

    snprintf(sql, sizeof(sql),
             "update friend_request set status=%d, handled_at=now() where id=%lld and status=0",
             accept ? 1 : 2, (long long)reqid);
    bool ok = mysql.update(sql);

    if (ok && accept)
    {
        char sql1[1024] = {0};
        char sql2[1024] = {0};
        snprintf(sql1, sizeof(sql1), "insert ignore into friend(userid,friendid) values(%d,%d)", fromid, toid);
        snprintf(sql2, sizeof(sql2), "insert ignore into friend(userid,friendid) values(%d,%d)", toid, fromid);
        ok = mysql.update(sql1) && mysql.update(sql2);
    }

    if (ok)
    {
        mysql.update("commit");
        return 0;
    }

    mysql.update("rollback");
    return 6;
}
