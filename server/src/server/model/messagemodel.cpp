#include "messagemodel.hpp"
#include "db.h"
#include "public.hpp"
#include "json.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
using json = nlohmann::json;

long long MessageModel::insert(int fromid, int toid, int groupid, int type, const string &content)
{
    long long mid = 0;
    MySQL mysql;
    if (mysql.connect())
    {
        string escaped = mysql.escape(content);

        string sql = "insert into message(from_id,to_id,group_id,type,content) values(" +
                     to_string(fromid) + "," + to_string(toid) + "," + to_string(groupid) + "," +
                     to_string(type) + ",'" + escaped + "')";

        if (mysql.update(sql))
        {
            mid = (long long)mysql_insert_id(mysql.getConnection());
        }
    }
    return mid;
}

vector<string> MessageModel::queryPrivateHistory(int userid, int friendid, long long beforeId, int limit)
{
    vector<string> vec;

    // 用数组下标游标分页，避免 LIMIT offset 越翻越慢
    string cursor = (beforeId > 0) ? (" and m.id < " + to_string(beforeId)) : "";

    char sql[2048] = {0};
    snprintf(sql, sizeof(sql),
             "select m.id,m.from_id,u.name,m.content,m.type,"
             "date_format(m.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') "
             "from message m join user u on u.id=m.from_id "
             "where m.group_id=0 and ((m.from_id=%d and m.to_id=%d) or (m.from_id=%d and m.to_id=%d))"
             "%s order by m.id desc limit %d",
             userid, friendid, friendid, userid, cursor.c_str(), limit);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                json msg;
                msg["mid"] = atoll(row[0]);   // 消息行 id，供客户端作为 before_id 游标
                msg["id"] = atoi(row[1]);
                msg["name"] = row[2] ? row[2] : "";
                msg["msg"] = row[3] ? row[3] : "";
                msg["type"] = atoi(row[4]);
                msg["time"] = row[5] ? row[5] : "";
                vec.push_back(msg.dump());
            }
            mysql_free_result(res);
        }
    }

    // 查询是倒序取最近 N 条，返回给客户端前转成正序
    reverse(vec.begin(), vec.end());
    return vec;
}

vector<string> MessageModel::queryGroupHistory(int groupid, long long beforeId, int limit)
{
    vector<string> vec;

    string cursor = (beforeId > 0) ? (" and m.id < " + to_string(beforeId)) : "";

    char sql[2048] = {0};
    snprintf(sql, sizeof(sql),
             "select m.id,m.from_id,u.name,m.content,m.type,"
             "date_format(m.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') "
             "from message m join user u on u.id=m.from_id "
             "where m.group_id=%d%s order by m.id desc limit %d",
             groupid, cursor.c_str(), limit);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                json msg;
                msg["mid"] = atoll(row[0]);
                msg["id"] = atoi(row[1]);
                msg["name"] = row[2] ? row[2] : "";
                msg["msg"] = row[3] ? row[3] : "";
                msg["type"] = atoi(row[4]);
                msg["time"] = row[5] ? row[5] : "";
                msg["groupid"] = groupid;
                vec.push_back(msg.dump());
            }
            mysql_free_result(res);
        }
    }

    reverse(vec.begin(), vec.end());
    return vec;
}

vector<string> MessageModel::queryFramesByIds(const vector<long long> &ids)
{
    vector<string> vec;
    if (ids.empty())
    {
        return vec;
    }

    // 拼成 in (1,2,3) 的形式；id 都是服务端自己生成的长整型，不存在注入风险
    string inList;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
        {
            inList += ",";
        }
        inList += to_string(ids[i]);
    }

    char sql[4096] = {0};
    snprintf(sql, sizeof(sql),
             "select m.id,m.from_id,m.to_id,m.group_id,u.name,m.content,m.type,"
             "date_format(m.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') "
             "from message m join user u on u.id=m.from_id "
             "where m.id in (%s) order by m.id asc",
             inList.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                int groupid = atoi(row[3]);

                json msg;
                // 和在线转发的帧保持同一套字段，客户端不用区分在线/离线
                msg["msgid"] = (groupid > 0) ? GROUP_CHAT_MSG : ONE_CHAT_MSG;
                msg["mid"] = atoll(row[0]);
                msg["id"] = atoi(row[1]);
                msg["to"] = atoi(row[2]);
                if (groupid > 0)
                {
                    msg["groupid"] = groupid;
                }
                msg["name"] = row[4] ? row[4] : "";
                msg["msg"] = row[5] ? row[5] : "";
                msg["type"] = atoi(row[6]);
                msg["time"] = row[7] ? row[7] : "";
                vec.push_back(msg.dump());
            }
            mysql_free_result(res);
        }
    }

    return vec;
}

vector<string> MessageModel::querySessions(int userid, int limit)
{
    vector<string> vec;
    if (userid <= 0)
    {
        return vec;
    }
    if (limit <= 0)
    {
        limit = 50;
    }
    if (limit > 200)
    {
        limit = 200;
    }

    // 每个会话(好友或群)只看最后一条：单聊按对方分组取 max(id)，群聊按 group_id 分组取 max(id)
    char sql[4096] = {0};
    snprintf(sql, sizeof(sql),
             "select * from ("
             "("
             "select m.id as mid,m.content as msg,m.type,"
             "date_format(m.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') as time,"
             "if(m.from_id=%d,m.to_id,m.from_id) as conv_id,"
             "u.name as conv_name,0 as groupid "
             "from message m "
             "join user u on u.id=if(m.from_id=%d,m.to_id,m.from_id) "
             "where m.group_id=0 and m.id in ("
             "select max(id) from message where group_id=0 and (from_id=%d or to_id=%d) "
             "group by if(from_id=%d,to_id,from_id))"
             ")"
             " union all "
             "("
             "select m.id as mid,m.content as msg,m.type,"
             "date_format(m.created_at,'%%Y-%%m-%%d %%H:%%i:%%s') as time,"
             "m.group_id as conv_id,g.groupname as conv_name,m.group_id as groupid "
             "from message m "
             "join allgroup g on g.id=m.group_id "
             "where m.group_id>0 and m.id in ("
             "select max(id) from message "
             "where group_id in (select groupid from groupuser where userid=%d) "
             "group by group_id)"
             ")"
             ") t order by mid desc limit %d",
             userid, userid, userid, userid, userid, userid, limit);

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
                item["mid"] = atoll(row[0]);
                item["msg"] = row[1] ? row[1] : "";
                item["type"] = atoi(row[2]);
                item["time"] = row[3] ? row[3] : "";
                item["id"] = atoi(row[4]);       // 会话 id：单聊为好友 id，群聊为 group_id
                item["name"] = row[5] ? row[5] : "";
                item["groupid"] = atoi(row[6]);   // 单聊固定 0，群聊为 group_id
                vec.push_back(item.dump());
            }
            mysql_free_result(res);
        }
    }
    return vec;
}
