#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

//json 序列化 示例1
string func1() {
    json js;
    js["id"] = {1,2,3,4,5};
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing";
    string sendBuf = js.dump();
    // cout<<js<<endl;
    cout<<sendBuf.c_str()<<endl;
    char *str  =const_cast<char *>(sendBuf.c_str());
    cout<<str<<endl;
    return sendBuf;
}

//json 序列化 示例2  json 可以序列化更复杂的东西  自己
string func2() {
    json js;
    js["student"] = {{"xiaojiner",12},{"xiaojin",12}};
    string sendBuf = js.dump();
    // cout<<js<<endl;
    cout<<sendBuf.c_str()<<endl;
    //char *str  =const_cast<char *>(sendBuf.c_str());
    return sendBuf;
}


//json 序列化 示例3 序列化容器
string func3() {
    json js;
    
    // 直接序列化 一个vector 容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    js["list"] = vec;

    // 直接序列化 一个map容器
    map<int,string> m;
    m.insert({1,"xiaojin"});
    m.insert({2,"huiling"});
    js["path"] = m;

    string sendBuf = js.dump();
    // cout<<js<<endl;
    cout<<sendBuf.c_str()<<endl;
    return sendBuf;
}

int main() {
    string recvBuf = func2();
    json js = json::parse(recvBuf);
    json js2 = js["student"];
    cout<<js2["xiaojin"]<<endl;
    cout<<endl;
    return 0;
}
