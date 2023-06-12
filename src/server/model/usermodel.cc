
#include "db.h"
#include <iostream>
#include "usermodel.hpp"
#include "dbpool.h"
using namespace std;

// User 表的增加方法
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "insert into user(name,password,state) values('%s' , '%s' , '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    {
        if (mysql->update(sql))
        {
            // 获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql->getConnection()));
            return true;
        }
    }
    return false;
}

// 根据 id 查询信息
User UserModel::query(int id)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "select * from user where id = %d", id);

    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    { // 连接成功
        MYSQL_RES *res = mysql->query(sql);
        if (res != nullptr)
        { // 查询成功
            MYSQL_ROW row = mysql_fetch_row(res);
            LOG_INFO << "query " << sql << " success!";
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    return User();
}

// 更新用户的状态信息
bool UserModel::update(User user)
{
    char sql[1024] = {0};
    // 组装sql语句
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    {
        if (mysql->update(sql))
        {
            // 修改状态成功
            return true;
        }
    }
    return false;
}

// 重置用户state
void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online' ";
    auto mysql = CommonConnectionPool::getInstance()->getConnection();
    if (mysql)
    {
        if (mysql->update(sql))
        {
        }
    }
}