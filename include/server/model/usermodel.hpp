#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    //注册用户
    bool insert(User& user);

    //根据 id 查询信息
    User query(int id);

    //更新用户的状态信息
    bool update(User user);

    //重置用户state
    void resetState();
protected:
private:
};

#endif