#include "chatserver.hpp"
#include <iostream>
#include "usermodel.hpp"
#include "user.hpp"
#include "offlinemessagemodel.hpp"
#include "chatservice.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
//这个文件  用来处理 服务器用ctrl + c 挂掉
#include <signal.h>
using namespace std;

//  处理服务器 ctrl + c 结束后， 重置user的状态信息
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc, char ** argv)
{
    //FriendModel f;
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT,resetHandler);
    EventLoop loop; // epoll
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start(); // listenfd epoll_ctl -> epoll
    loop.loop();    // epoll_wait
    return 0;
}