#include "usermodel.hpp"
#include <iostream>
#include "db.h"
using namespace std;

bool UserModel::insert(User &user)
{
    MySQL mysql;
    if (mysql.connect())
    {
        string name = mysql.escape(user.getName());
        string pwd = mysql.escape(user.getPwd());

        char sql[2048] = {0};
        snprintf(sql, sizeof(sql), "insert into user(name,password,state) values('%s','%s','%s')",
                 name.c_str(), pwd.c_str(), user.getState().c_str());

        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                user.setAvatar(row[4] == nullptr ? "" : row[4]);
                mysql_free_result(res);
                return user;
            }
            mysql_free_result(res);
        }
    }
    return User();
}

bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
     char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect())
    {
       mysql.update(sql);
    }
}

bool UserModel::updatePwd(int id, const string &pwd)
{
    char sql[1024] = {0};
    MySQL mysql;
    if (mysql.connect())
    {
        string safe = mysql.escape(pwd);
        snprintf(sql, sizeof(sql), "update user set password = '%s' where id = %d", safe.c_str(), id);
        return mysql.update(sql);
    }
    return false;
}
