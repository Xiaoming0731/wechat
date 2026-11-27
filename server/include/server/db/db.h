#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
using namespace std;

class MySQL
{
public:
    MySQL();
    ~MySQL();
    bool connect();
    bool update(string sql);
    MYSQL_RES *query(string sql);
    MYSQL* getConnection();
    // 转义字符串中的特殊字符，必须在 connect() 成功之后调用
    string escape(const string &s);

private:
    MYSQL *_conn;
};

#endif
