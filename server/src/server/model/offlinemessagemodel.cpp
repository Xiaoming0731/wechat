#include "offlinemessagemodel.hpp"
#include "db.h"
#include <cstdio>
#include <cstdlib>

void OfflineMsgModel::insert(int userid, long long mid)
{
    if (userid <= 0 || mid <= 0)
    {
        return;
    }

    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "insert into offlinemessage(userid,mid) values(%d,%lld)",
             userid, mid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

vector<long long> OfflineMsgModel::query(int userid)
{
    vector<long long> vec;

    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "select mid from offlinemessage where userid=%d order by mid asc",
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
                vec.push_back(atoll(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return vec;
}

void OfflineMsgModel::remove(int userid, long long maxMid)
{
    if (userid <= 0 || maxMid <= 0)
    {
        return;
    }

    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "delete from offlinemessage where userid=%d and mid<=%lld",
             userid, maxMid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
