#ifndef CHATSERVER_H
#define CHATSERVER_H

// 引入 muduo库需要的头文件
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

#include <string>

using namespace muduo;
using namespace muduo::net;

// 定义ChatServer类
class ChatServer
{
public:
// 初始化
    ChatServer(EventLoop *loop,const InetAddress & listenAddr,const string& nameArg);
    ~ChatServer();

//启动服务
    void start();
protected:
private:
    //设置回调相关的回调函数
    //设置连接相关
    void onConnection(const TcpConnectionPtr&);
    //设置读写相关
    void onMessage(const TcpConnectionPtr&,Buffer*,Timestamp);


    TcpServer _server; //组合的muduo 库， 实现服务器功能的类对象
    EventLoop *_loop; //指向事件循环的对象的指针 
};
#endif