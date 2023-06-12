#include <iostream>
#include "db.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>
using namespace std;

#pragma once
/*
实现连接池功能模块
*/

class CommonConnectionPool {
public:
	static CommonConnectionPool* getInstance();
	//bool loadConfigFile(); //加载配置文件

	//给外部提供接口，从空间的连接池中获得一个连接 使用智能指针的好处就是，当计数为0的时候，才调用析构函数
	shared_ptr<MySQL> getConnection();
private:
	CommonConnectionPool();//构造函数私有化
	~CommonConnectionPool();
	bool loadConfigFile(); //加载配置文件
	
	void produceConnectionTask();//运行在独立的线程中，专门负责生产新的线程
	void scannerConnectionTask();//定时扫描超过maxIdleTime的空闲线程，进行资源回收
	//这一部分应该放在配置文件
	string _ip; //ip地址
	unsigned short _port;//端口号
	string _name; //登录用户名
	string _password;//登录密码
	string _dbname; //连接数据库名字
	int _initSize;//初始化数量
	int _maxSize;//最大连接数量
	int _maxIdleTime;//连接池最大空闲时间
	int _connectionTimeout;//连接池获取连接的超时时间

	static CommonConnectionPool* _poolPtr;
	queue<MySQL*> _connectionQueue;//连接队列
	mutex _queueMutex;//访问queue的队列
	atomic_int _connectionCnt;//记录连接所创建的connection连接的总数量
	condition_variable cv; //设置条件变量
};