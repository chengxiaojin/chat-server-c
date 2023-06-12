#include "friendmodel.hpp"
#include "db.h"
#include "dbpool.h"
#include <vector>
// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "insert into friend  values(%d , %d)",
            userid,friendid);
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql != nullptr)
    {
        if (mysql->update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            
        }
    }
}

// 返回用户的好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",
    userid);
    vector<User> vec;
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql!= nullptr)
    { // 连接成功
        MYSQL_RES *res = mysql->query(sql);
        if (res != nullptr)
        { // 查询成功
            MYSQL_ROW row ;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
            }
            
            mysql_free_result(res);
        }
    }
    return vec;

}