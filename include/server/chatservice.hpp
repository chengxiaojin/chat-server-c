#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include <mutex>
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

// 处理消息的事件回调操作
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的函数接口
    static ChatService *instance();
    // 处理登录的业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取处理注册的业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 处理单点的聊天
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理用户注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常后 业务重置方法
    void reset();

    // 群组业务
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

protected:
private:
    ChatService();

    // 存储消息id 和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储 在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据类操作对象
    // 操作user
    UserModel _userModel;
    // 操作离线消息的推送
    OfflineMsgModel _offlineMsg;
    // 操作friend
    FriendModel _friednModel;
    // 操作组信息
    GroupModel _groupModel;

    //操作Redis 对象
    Redis _redis;
};
#endif