#ifndef GROUPMODEL_H
#define GROUPMODEL_H


#include "group.hpp"
#include <string>
#include <vector>
using namespace std;


//维护组群信息操作的接口
class GroupModel {

public:
    //创建组群
    bool createGroup(Group &group);

    //加入组群
    void addGroup(int userid, int groupid , string role);

    //查询用户所在组群的信息
    vector<Group> queryGroup(int userid);

    //根据指定的groupid 查询群组用户id列表，除userid自己， 主要用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
    
protected:
private:
};


#endif