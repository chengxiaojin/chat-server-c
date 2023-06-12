#include "chatserver.hpp"
#include <functional>
#include <iostream>
#include "chatservice.hpp"
#include "json.hpp"

using namespace std;
using namespace placeholders;
using json =  nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 给服务器注册用户创建连接和断开的回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 给服务器注册用户读写时间回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务器的线程数量
    _server.setThreadNum(4);
}

// 专门处理用户的连接的创建和断开
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() 
        << "state : online" << std::endl;
    }
    else
    {
        std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() 
        << "state : offline" << std::endl;
        ChatService::instance()->clientCloseException(conn);
        //用户断开连接
        conn->shutdown();
        //_loop->quit(); // 等于关闭epoll
    }
}

// 专门处理用户的读写事件的
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buff,
                           Timestamp time)
{
    string buf = buff->retrieveAllAsString();
    //数据的反序列化
     json js = json::parse(buf);

    //达到目的： 完全解耦网络模块的代码 和 业务模块的代码
    // 通过js["msgid"] -> handler ->conn js time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相关操作
    msgHandler(conn, js , time);
    // std::cout << "recv data: " << buf << " time " << time.toString() << std::endl;
    // conn->send(buf);
}

void ChatServer::start()
{
    _server.start();
}

ChatServer::~ChatServer()
{
}