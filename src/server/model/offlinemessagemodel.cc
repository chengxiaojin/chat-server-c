#include "offlinemessagemodel.hpp"
#include <muduo/base/Logging.h>
#include "db.h"
#include "dbpool.h"
using namespace muduo;

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "insert into offlinemessage values(%d ,'%s')",
            userid,msg.c_str());
   auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    {
        if (mysql->update(sql))
        {
            // 插入offlinemessage 表成功
            LOG_INFO<< "insert offline message success!";
        }
    }
}
// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "delete from offlinemessage where userid = %d",
            userid);
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    {
        if (mysql->update(sql))
        {
            // 插入offlinemessage 表成功
            LOG_INFO<< "delete offlinemessage id = " << userid << "success!";
        }
    }
}
// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "select message  from offlinemessage where userid = %d", userid);

    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    vector<string> vec; //存储查询返回结果
    if (mysql)
    { // 连接成功
        MYSQL_RES *res = mysql->query(sql);
        if (res != nullptr)
        { // 查询成功
            MYSQL_ROW row;
            while ((row  = mysql_fetch_row(res)) != nullptr) {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            //LOG_INFO<< "query offlinemessage id = " << userid << "success!";
        }
    }
    return  vec;
}