#include "db.h"
#include <muduo/base/Logging.h>
#include <vector>
#include <cstdlib>

// 默认值兼容本地开发，容器部署时通过环境变量覆盖
static string envOr(const char *key, const char *def)
{
    const char *v = getenv(key);
    return (v != nullptr && *v != '\0') ? string(v) : string(def);
}

static string server = envOr("DB_HOST", "127.0.0.1");
static string user = envOr("DB_USER", "root");
static string password = envOr("DB_PASS", "123456");
static string dbname = envOr("DB_NAME", "chat");

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names utf8mb4");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql failed!";
    }
    return p;
}

bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

MYSQL* MySQL::getConnection(){
    return _conn;
}

string MySQL::escape(const string &s)
{
    if (_conn == nullptr)
    {
        return s;
    }

    vector<char> buf(s.size() * 2 + 1);
    unsigned long len = mysql_real_escape_string(_conn, buf.data(), s.c_str(), (unsigned long)s.size());
    return string(buf.data(), len);
}