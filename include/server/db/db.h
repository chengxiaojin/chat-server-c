#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>
#include <muduo/base/Logging.h>
using namespace std;
using namespace muduo;

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "clj";
static string password = "199821";
static string dbname = "chat";
// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL();
   
    // 释放数据库连接资源
    // 这里用UserModel示例，通过UserModel如何对业务层封装底层数据库的操作。代码示例如下：
    ~MySQL();
    
    // 连接数据库
    bool connect(string ip, unsigned short port, string user, string password,
	string dbname);
    
    // 更新操作
    bool update(string sql);
    
    // 查询操作
    MYSQL_RES *query(string sql);
    

    //用于释放空闲连接
    void refreshAliveTime() { _alivetime = clock(); }
	clock_t getAliveTime() { return clock() - _alivetime; }
    MYSQL* getConnection() {return _conn;}
private:
    MYSQL *_conn;
    clock_t _alivetime;//记录进到空闲状态的后的起始存活时间
};

#endif
