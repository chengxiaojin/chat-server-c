#include <iostream>
#include <fstream>
#include <mutex>
#include "dbpool.h"
#include "db.h"
#include <memory>
#include <thread>
#include <functional>
#include <muduo/base/Logging.h>
using namespace std;
using namespace muduo;

CommonConnectionPool::CommonConnectionPool() {//构造函数私有化
	//加载配置文件
	if (!loadConfigFile()) 
		return; 

	//创建初始的连接数量
	for (int i = 0; i < _initSize; ++i) {
		MySQL* p = new MySQL();
		bool flag = p->connect(_ip, _port, _name, _password, _dbname);
		if (!flag) {
			LOG_INFO<<"初始化创建链接失败";
		}
		_connectionQueue.push(p);
		p->refreshAliveTime();//刷新一下进入空闲的起始时间
		_connectionCnt++;
	}

	//启动一个新线程， 作为生产者的连接
	thread produce(bind( & CommonConnectionPool::produceConnectionTask, this));
	//设置为守护线程 --> 当前主线程完了，自动结束
	produce.detach();

	//启动一个新线程 ，定时扫描超过maxIdleTime的空闲线程，进行资源回收
	thread scanner(bind(&CommonConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}
//生产任务
void CommonConnectionPool::produceConnectionTask() {
	for (;;) {
		unique_lock<mutex> lck(_queueMutex);
		while (!_connectionQueue.empty()) {
			cv.wait(lck);//队列不空，此处生产线程进入等待状态
		}

		//连接数量没有到达上限，继续创建连接
		if (_connectionCnt < _maxSize) {
			MySQL* p = new MySQL();
			p->connect(_ip, _port, _name, _password, _dbname);
			_connectionQueue.push(p);
			p->refreshAliveTime();//刷新一下进入空闲的起始时间
			_connectionCnt++;
		}

		//通知所以正在等待的线程
		cv.notify_all();
	}
}

//定时扫描
void CommonConnectionPool::scannerConnectionTask() {
	for (;;) {
		//通过sleep 模拟睡眠效果
		this_thread::sleep_for(chrono::milliseconds(_maxIdleTime));
		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lck(_queueMutex);
		while (_connectionCnt > _initSize) {//如果大于初始量，才进行释放
			MySQL* p = _connectionQueue.front();
			if (p->getAliveTime() > _maxIdleTime * 1000) {
				_connectionQueue.pop();
				--_connectionCnt;
				delete p;
			}
			else {
				break;//如果连队头的连接都没有大于的话，那就没有空闲多久
			}
		}
	}
}

CommonConnectionPool::~CommonConnectionPool(){}

CommonConnectionPool * CommonConnectionPool::_poolPtr = nullptr;

mutex m;
CommonConnectionPool* CommonConnectionPool::getInstance() {
	if (_poolPtr == nullptr) {
		unique_lock<mutex> lck(m);
		if (_poolPtr == nullptr) {
			_poolPtr = new CommonConnectionPool();
		}
	}
	return _poolPtr;
}

bool CommonConnectionPool::loadConfigFile() {
	cout<<__FILE__<<endl;
	FILE* fptr = fopen("./../poolconf.ini", "r"); //打开配置文件
	if (!fptr) {
		LOG_ERROR <<"打开文件失败";
		return false;
	}
	while (!feof(fptr)) {
		char line[1024] = { 0 }; //文件读取buffer
		fgets(line, 1024, fptr);//读取一行 第一个参数是buffer 第二个是大小 第三个是文件指针
		string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) continue;//这一行不包含配置文件

		int endidx = str.find('\n', 0);
		string key = str.substr(0, idx);
		string val = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip") {
			_ip = val;
		}
		else if (key == "port") {
			_port = atoi(val.c_str());
		}
		else if (key == "name") {
			_name = val;
		}
		else if (key == "password") {
			_password = val;
		}
		else if (key == "initSize") {
			_initSize = atoi(val.c_str());
		}
		else if (key == "maxSize") {
			_maxSize = atoi(val.c_str());
		}
		else if (key == "maxIdleTime") {
			_maxIdleTime = atoi(val.c_str());
		}
		else if (key == "connectionTimeOut") {
			_connectionTimeout = atoi(val.c_str());
		}
		else if (key == "dbname") {
			_dbname = val;
		}
	}
	return true;
}

//给外部提供接口，从空间的连接池中获得一个连接 使用智能指针的好处就是，当计数为0的时候，才调用析构函数
shared_ptr<MySQL> CommonConnectionPool::getConnection() {
	unique_lock<mutex> lck(_queueMutex);
	while (_connectionQueue.empty()) {
		if (cv_status::timeout == cv.wait_for(lck, chrono::milliseconds(_connectionTimeout))) {
			if (_connectionQueue.empty()) {
				LOG_INFO << "获取空闲连接超时......获取连接失败";
				return nullptr;
			}
		}
	}

	//智能指针 完了过后，会调用Connection 的析构函数 ， 所以 要设计自己的Deleter
	shared_ptr<MySQL> sp(_connectionQueue.front(),
		[&](MySQL* conn)->void {
			unique_lock<mutex> lck(_queueMutex);
			_connectionQueue.push(conn);
			conn->refreshAliveTime();//刷新一下进入空闲的起始时间

		});
	_connectionQueue.pop();
	if (_connectionQueue.empty())
	{//谁消费了队列里面的最后一个连接， 谁负责通知生产者生产
		cv.notify_all();
	}
	return sp;
}
