#include "db.h"

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
// 这里用UserModel示例，通过UserModel如何对业务层封装底层数据库的操作。代码示例如下：
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect(string ip, unsigned short port, string user, string password,
	string dbname)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p != nullptr)
    {
        // c 和 c++ 代码默认的编码字符集是ASCII，如果不设置，从MySQL 上拉下来的中文 显示 ?
        mysql_query(_conn, "set names gbk");
        LOG_INFO << " connect mysql success!";
    }
    else
    {
        LOG_INFO << " connect mysql fail!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << " : "
                 << sql << " 更新失败!";
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
                 << sql << " 查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
