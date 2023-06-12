#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前用户登录的好友列表
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表消息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间
string getCurrentTime();

// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现 ， main 线程作为发送线程   子线程作为接收线程
int main(int argc, char **argv)
{

    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 192.168.89.129 6000";
        ::exit(-1);
    }

    // 通过解析命令行参数传递的ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建客户端用于通信的 clientfd socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        ::exit(-1);
    }

    // 填写 client 需要连接的server信息的ip + port
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof serverAddr);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    // 使用 clientfd socket 和 server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&serverAddr, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        ::exit(-1);
    }

    // main 线程用于接收用户的输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录 注册 退出
        cout << "-----------------------------------" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. exit" << endl;
        cout << "-----------------------------------" << endl;

        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区 残留的回车

        switch (choice)
        {
        case 1: // login 业务
        {
            int id = 0;
            char pwd[20] = {0};
            cout << "userid: ";
            cin >> id;
            cin.get(); // 处理掉缓存回车
            cout << "user password: ";
            cin.getline(pwd, 20);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send login msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    //cout << responsejs["errno"] << endl;
                    cout << buffer << endl;
                    if (0 != responsejs["errno"].get<int>())
                    { // 登录失败
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        // 登录成功
                        // 记录 当前用户的id 和 name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表消息
                        if (responsejs.contains("friends"))
                        {
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前的组信息
                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json gpjs = json::parse(groupstr);
                                Group group;
                                group.setId(gpjs["id"].get<int>());
                                group.setName(gpjs["groupname"]);
                                group.setDesc(gpjs["groupdesc"]);

                                vector<string> vec2 = gpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    json js = json::parse(userstr);
                                    GroupUser user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示用户登录信息
                        showCurrentUserData();

                        // 显示当前用户的离线信息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                // time + [id] + name + "said : " + xxx
                                if (responsejs.contains("msgid") && ONE_CHAT_MSG== js["msgid"].get<int>())
                                {
                                    cout << js["time"] << "[" << js["id"] << "] " << js["name"]
                                         << " said: " << js["msg"] << endl;
                                }
                                else
                                {
                                    cout << "群消息 [ " << js["groupid"] << " ] " << js["time"]
                                         << " [ " << js["id"] << " ] "
                                         << " said: " << js["msg"] << endl;
                                }
                            }
                        }

                        // 登录成功  启动 接收线程 负责接收数据
                        std::thread readTask(readTaskHandler, clientfd);
                        readTask.detach();

                        // 进入聊天主页面
                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2: // register业务
        {
            char name[30];
            char pwd[20];
            cout << "user name: ";
            cin.getline(name, 30);
            cout << "user password: ";
            cin.getline(pwd, 20);
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << "name"
                             << " is already exist, register error!" << endl;
                    }
                    else
                    {
                        cout << name << " register success, userid is " << responsejs["id"] << ", do not forget it!" << endl;
                    }
                }
            }
            break;
        }
        case 3:
        { // exit 业务
            close(clientfd);
            exit(0);
        }
        default:
        {
            cerr << "invalid input" << endl;
            break;
        }
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "++++++++++++++++++++++++++++++++++++++login user++++++++++++++++++++++++++++++++++++++" << endl;
    cout << "current login user -> id :" << g_currentUser.getId() << " name : " << g_currentUser.getName() << endl;
    cout << "+++++++++++++++++++++++++++++++++++++friend list++++++++++++++++++++++++++++++++++++++" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }

    cout << "+++++++++++++++++++++++++++++++++++++group list+++++++++++++++++++++++++++++++++++++++" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }

    cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
}

// 获取系统当前时间
string getCurrentTime()
{
    // 获取当前时间的 Unix 时间戳
    std::time_t now = std::time(nullptr);

    // 将 Unix 时间戳转换为本地时间
    std::tm *local_time = std::localtime(&now);

    // 获取年、月、日、时、分、秒等信息
    int year = local_time->tm_year + 1900;
    int month = local_time->tm_mon + 1;
    int day = local_time->tm_mday;
    int hour = local_time->tm_hour;
    int minute = local_time->tm_min;
    int second = local_time->tm_sec;

    // 输出当前时间
    char curtime[50] = {0};
    sprintf(curtime, "Current time: %d - %d - %d %d:%d:%d", year, month, day, hour, minute, second);
    return curtime;
}
string getCurrentTime();
// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            ::exit(-1);
        }
        json js = json::parse(buffer);
        if (ONE_CHAT_MSG == js["msgid"].get<int>())
        {
           // {"id":28,"msg":"xiao baobei","msgid":5,"name":"xiao jin","time":"Current time: 2023 - 6 - 8 17:33:9","toid":13}
            // 处理一对一信息聊天
            if (js.contains("errno") && USER_OFFLINE == js["errno"].get<int>())
            {
                cout << js["offlineresponse"] << endl;
            }
            else
            {
                cout << js["time"].get<string>() << " [ " << js["id"] << " ] "
                     << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            }
            continue;
        }
        if (LOGINOUT_MSG == js["msgid"].get<int>())
        {
            // 处理 退出登录业务
            if (0 == js["errno"].get<int>())
            {
                // 退出成功
                g_currentUser = User();
                g_currentUserFriendList = vector<User>();
                g_currentUserGroupList = vector<Group>();
                return;
            }
            else
            {
                // 退出失败
                cout << js["msg"] << endl;
            }
        }
        if (GROUP_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << "群消息 [ " << js["groupid"] << " ] " << js["time"]
                 << " [ " << js["id"] << " ] "
                 << " said: " << js["msg"] << endl;
            continue;
        }
    }
}

// 登录用户后的操作函数
void help(int id = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string str = "");

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "display all help commands, format-> help"},
    {"chat", "one to one chat, format-> chat:friendid:message"},
    {"addfriend", "add friend, format-> addfriend:friendid"},
    {"creategroup", "create group, format-> creategroup:groupname:groupdesc"},
    {"addgroup", "add group, format-> addgroup:groupid"},
    {"groupchat", "group chat, format-> groupchat:groupid:message"},
    {"loginout", "loginout, format-> loginout"}};

// 注册系统 支持的客户端 命令处理
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    for (;;)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "Invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件回调
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
        if (it->first == "loginout")
            return;
    }
}

// help command
void help(int id, string str)
{
    cout << " show command list >>> " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// add friend command handler   ->id
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

// creategroup command  -> groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// add friend command handler   ->groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// groupchat   -> groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    js["name"] = g_currentUser.getName();
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "some error occurs!" << endl;
        exit(-1);
    }
}