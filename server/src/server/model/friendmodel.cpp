#include "friendmodel.hpp"
#include "db.h"

void FriendModel::insert(int userid, int friendid)
{
    char sql1[1024] = {0};
    snprintf(sql1, sizeof(sql1), "insert ignore into friend(userid,friendid,remark) values(%d,%d,'')", userid, friendid);
    char sql2[1024] = {0};
    snprintf(sql2, sizeof(sql2), "insert ignore into friend(userid,friendid,remark) values(%d,%d,'')", friendid, userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql1);
        mysql.update(sql2);
    }

}

bool FriendModel::isFriend(int userid, int friendid)
{
    if (userid <= 0 || friendid <= 0 || userid == friendid)
    {
        return false;
    }

    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select 1 from friend where userid=%d and friendid=%d limit 1", userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            bool exists = (mysql_fetch_row(res) != nullptr);
            mysql_free_result(res);
            return exists;
        }
    }
    // 查询失败时按"非好友"处理，宁可拦掉也不放行
    return false;
}

vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state,a.avatar,b.remark from user a join friend b on a.id=b.friendid where b.userid = %d", userid);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setAvatar(row[3] == nullptr ? "" : row[3]);
                user.setRemark(row[4] == nullptr ? "" : row[4]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}

bool FriendModel::updateRemark(int userid, int friendid, const string &remark)
{
    if (userid <= 0 || friendid <= 0 || userid == friendid)
    {
        return false;
    }
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql),
             "update friend set remark='%s' where userid=%d and friendid=%d",
             remark.c_str(), userid, friendid);

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}