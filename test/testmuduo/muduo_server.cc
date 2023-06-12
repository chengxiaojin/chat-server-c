/*
使用 muduo网络库 编写一个chatserver 和 chatclient

epoll + 线程池
好处 ： 能够把网络I/O 的代码和业务代码区分开
暴露的只有 ： 用户的连接/断开  用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace muduo;
using namespace muduo::net;

// 基于 muduo网络库 开发服务器程序
/*
1 创建 Tcpserver 对象
2 创建 EventLoop事件循环对象指针
3 明确Tcpserver 的构造函数
4 在当前服务器类的构造函数当中 注册处理连接的回调函数和用户读写的回调函数
5 设置服务器端的线程数量 muduo库会自己划分io线程 和 worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户创建连接和断开的回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写时间回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

        //设置服务器的线程数量
        _server.setThreadNum(4); 
    }
    void start() {
        _server.start();
    }
private:
    // 专门处理用户的连接的创建和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state : online" << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << "state : offline" << std::endl;
            conn->shutdown();
            //_loop->quit(); // 等于关闭epoll
        }
    }
    // 专门处理用户的读写事件的
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buff,
                   Timestamp time)
    {
        string buf = buff->retrieveAllAsString();
        std::cout<< "recv data: "<<buf<<" time " << time.toString()<<std::endl;
        conn->send(buf);
    }
    TcpServer _server; // #1
    EventLoop *_loop;  // #2 epoll
};


int main () {
    EventLoop loop; //epoll
    InetAddress addr("192.168.89.129",6000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start(); //listenfd epoll_ctl -> epoll
    loop.loop(); // epoll_wait

    return 0;
}