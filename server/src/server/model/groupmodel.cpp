#include "db.h"
#include "groupmodel.hpp"

bool GroupModel::createGroup(Group &group)
{
    MySQL mysql;
    if (mysql.connect())
    {
        string name = mysql.escape(group.getName());
        string desc = mysql.escape(group.getDesc());

        char sql[2048] = {0};
        snprintf(sql, sizeof(sql), "insert into allgroup(groupname,groupdesc) values('%s','%s')",
                 name.c_str(), desc.c_str());

        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    // insert ignore：已经在群里时不算失败
    // 显式写列名：groupuser 上还有 remark 列，依赖位置插入以后容易错位
    snprintf(sql, sizeof(sql), "insert ignore into groupuser(groupid, userid, grouprole) values(%d, %d, '%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql);
    }
    return false;
}

bool GroupModel::isGroupExist(int groupid)
{
    if (groupid <= 0)
    {
        return false;
    }

    char sql[256] = {0};
    snprintf(sql, sizeof(sql), "select 1 from allgroup where id=%d limit 1", groupid);

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
    return false;
}

bool GroupModel::isMember(int userid, int groupid)
{
    if (userid <= 0 || groupid <= 0)
    {
        return false;
    }

    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select 1 from groupuser where groupid=%d and userid=%d limit 1", groupid, userid);

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
    // 查询失败按"不在群里"处理
    return false;
}

bool GroupModel::updateGroupRemark(int userid, int groupid, const string &remark)
{
    if (userid <= 0 || groupid <= 0)
    {
        return false;
    }

    MySQL mysql;
    if (mysql.connect())
    {
        string escaped = mysql.escape(remark);
        char sql[1024] = {0};
        snprintf(sql, sizeof(sql),
                 "update groupuser set remark='%s' where userid=%d and groupid=%d",
                 escaped.c_str(), userid, groupid);
        return mysql.update(sql);
    }
    return false;
}

vector<Group> GroupModel::queryGroup(int userid)
{
    char sql[1024] = {0};
    // b.remark 是"当前用户对这个群"的备注，所以只能按 b.userid 关联
    sprintf(sql, "select a.id,a.groupname,a.groupdesc,b.remark from allgroup a inner join groupuser b on a.id=b.groupid where b.userid=%d", userid);

    vector<Group> groupvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) 
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                group.setRemark(row[3] == nullptr ? "" : row[3]);
                groupvec.push_back(group);
            }
            mysql_free_result(res);
        }

        for (Group &group : groupvec)
        {
            sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid=a.id where b.groupid=%d", group.getId());

            MYSQL_RES *res2 = mysql.query(sql);
            if (res2 != nullptr)
            {
                MYSQL_ROW row2;
                while ((row2 = mysql_fetch_row(res2)) != nullptr)  
                {
                    GroupUser user;
                    user.setId(atoi(row2[0]));
                    user.setName(row2[1]);
                    user.setState(row2[2]);
                    user.setRole(row2[3]);
                    group.getUsers().push_back(user);
                }
                mysql_free_result(res2);
            }
        }
    }
    return groupvec;
}

vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid !=%d ", groupid,userid);

    vector<int> idvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idvec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idvec;
}