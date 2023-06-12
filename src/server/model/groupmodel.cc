#include "groupmodel.hpp"
#include "db.h"
#include "dbpool.h"
 //创建组群
    bool GroupModel::createGroup(Group &group){
        char sql[1024] = {0} ;
        sprintf(sql,"insert into allgroup (groupname, groupdesc) values('%s', '%s')",
        group.getName().c_str(),group.getDesc().c_str());

        auto mysql = CommonConnectionPool::getInstance()->getConnection();
        if (mysql) {
            if (mysql->update(sql)) {
                group.setId(mysql_insert_id(mysql.get()->getConnection()));
                return true;
            }
        }
        return false;
    }

    //加入组群
    void GroupModel::addGroup(int userid, int groupid , string role){
        char sql[1024] = {0} ;
        sprintf(sql,"insert into groupuser values(%d,%d, '%s')",
        groupid,userid,role.c_str());

        auto mysql = CommonConnectionPool::getInstance()->getConnection();
        if (mysql) {
            if (mysql->update(sql)) {
            }
        }
    }

    //查询用户所在组群的信息
    vector<Group> GroupModel::queryGroup(int userid){
        /**
         * 1 查询userid 在 groupuser表中 查询出该用户所属的 群组消息
         * 2 再根据 群组消息，查询 属于该群组的所有用户id ， 并且和user表进行多表联合查询
         * 查出用户的详细信息
        */

        char sql[1024] = {0};

        sprintf(sql,"select a.id , a.groupname, a.groupdesc from allgroup a inner join groupuser b \
        on  a.id = b.groupid where b.userid = %d",userid);

        auto mysql = CommonConnectionPool::getInstance()->getConnection();
        vector<Group> groupVec;
        if (mysql) {
            MYSQL_RES *res = mysql->query(sql);
            if (res != nullptr) {
                //查出所有userid 所有的群组信息
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr) {
                    Group group;
                    group.setId(atoi(row[0]));
                    group.setName(row[1]);
                    group.setDesc(row[2]);
                    groupVec.push_back(group);
                }
                mysql_free_result(res);
            }
        }

        // 查询群组的用户信息
        for (Group & group : groupVec) {
            memset(sql,0,sizeof sql);

            sprintf(sql,"select a.id, a.name ,a.state,b.grouprole from user a join groupuser b on\
            b.userid = a.id where groupid = %d",group.getId());
            MYSQL_RES *res = mysql->query(sql);
            if (res != nullptr) {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr) {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                }
                mysql_free_result(res);
            }
        }
        return groupVec;

    }

    //根据指定的groupid 查询群组用户id列表，除userid自己， 主要用户群聊业务给群组其他成员群发消息
    vector<int> GroupModel::queryGroupUsers(int userid, int groupid){
        char sql[1024] = {0};
        sprintf(sql,"select userid from groupuser where groupid = %d and userid != %d",
        groupid,userid);
        auto mysql = CommonConnectionPool::getInstance()->getConnection();
        vector<int>idVec;
        if (mysql) {
            MYSQL_RES *res = mysql->query(sql);
            if (res != nullptr) {
                //查出所有userid 所有的群组信息
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr) {
                    idVec.push_back(atoi(row[0]));
                }
                mysql_free_result(res);
            }
        }
        return idVec;
    }