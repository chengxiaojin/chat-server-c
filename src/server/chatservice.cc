#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include "usermodel.hpp"
#include <map>
#include <iostream>
using namespace std;
using namespace muduo;

// 获取单例对象的函数接口
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 服务器异常后 业务重置方法
void ChatService::reset()
{
    // 把所有online 的 状态设置为offline
    _userModel.resetState();
}

// 注册消息以及 对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHandlerMap.insert({LOGINOUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//处理用户注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int id = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) {
            User user;
            user.setId(id);
            _userModel.update(user);
            _userConnMap.erase(it->first);
            js["msgid"] = LOGINOUT_MSG;
            js["msg"] = "offline success! welcome next time.";
            js["errno"] = 0;

            //将订阅通道 取消
            _redis.unsubscribe(id);
            
        }else 
        {
            js["msgid"] = LOGINOUT_MSG;
            js["msg"] = "offline fail! please retry.";
            js["errno"] = 1;
        }
        string str = js.dump();
        conn->send(str);
    }
    
}


// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friednModel.insert(userid, friendid);
}

// 处理登录的业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do login service";
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录 不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "the user is online!";
            conn->send(response.dump());
            conn->send("\n");
        }
        else
        {
        
            {
                lock_guard<mutex> lock(_connMutex);
                // 登录成功 记录用户连接信息
                _userConnMap.insert({id, conn});
            }

            //id 用户登录成功后  向redis 订阅channel (id)
            _redis.subscribe(id);

            // 登陆成功 更新用户状态信息
            user.setState("online");
            _userModel.update(user);

            

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["id"] = user.getId();
            response["errno"] = 0;
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsg.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取用户的离线消息过后  需要删除离线消息
                _offlineMsg.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friednModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroup(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Invalide userid or password!";
        conn->send(response.dump());
    }
}
// 获取处理注册的业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do register service";
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的空处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:  " << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //客户端异常退出  也要取消订阅通道
    _redis.unsubscribe(user.getId());
    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.update(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string t = js.dump();
    std::cout<<"___________________________________________________"<<endl;
    cout<<t<<endl;

    int toId = js["toid"].get<int>(); // 获取对端的id
    // string fromName = js["name"];
    // int fromId = js["id"].get<int>();
    // 下一步，判断是否有连接
    auto it = _userConnMap.find(toId);
    if (it != _userConnMap.end())
    {
        // 这个时候 说明 有连接  用户在线 转发消息
        it->second->send(js.dump()); // 消息转发
        return;
    }
    else
    {
        //查询toid 是否在线
        User user = _userModel.query(toId);
        if (user.getId() != -1 && user.getState() == "online") {
            _redis.publish(toId,js.dump());
            return; 
        }
        // 这个时候 用户不在现 存储转发消息
        _offlineMsg.insert(toId, js.dump());
        js["errno"] = USER_OFFLINE;
        char tmp[50] = {0};
        sprintf(tmp,"userid : %d do not online, message has chached!",toId);
        js["offlineresponse"] = tmp;
        conn->send(js.dump());
    }
}

// 处理群组的业务接口
// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人的信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发消息
            it->second->send(js.dump());
        }
        else
        {
             User user = _userModel.query(id);
             if (user.getId() != -1 && user.getState() == "online") {
                _redis.publish(user.getId(),js.dump());
                continue;
             }
            // 存储离线消息
            _offlineMsg.insert(id, js.dump());
        }
    }
}


// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsg.insert(userid, msg);
}