#pragma once
/*
	实现连接池功能模块
	连接池类
*/
#include<string>
#include<iostream>
#include<queue>
#include<atomic>
#include<mutex>
#include<thread>
#include<memory>
#include<functional>
#include<condition_variable>

#include"Connection.h"

using namespace std;

class ConnectionPool {
public:
	// 获取连接池对象实例
	// 连接池只需要一个，单例模式设计
	static ConnectionPool* getConnectionPool();

	// 给外部提供接口,从连接池获取连接
	shared_ptr<Connection> getConnection();


private:
	ConnectionPool();
	bool loadConfigFile();             // 加载配置文件 mysql.ini
	void produceConnectionTask();      // 用来生产新连接的，所以需独立开一个线程
	void scannerConnectionTask();      // 启动新定时线程， 扫描超过maxIdletime的空闲连接

	string _ip;                        // MySQL 对应 ip
	unsigned short _port;              // 端口号 : 3306
	string _username;                  // MySQL 对应 用户名
	string _password;                  // MySQL 对应 密码
	string _dbname;                    // MySQL 对应 名称
	int _initSize;                     // 连接池的初始连接数
	int _maxSize;                      // 连接池的最大连接数
	int _maxIdleTime;                  // 连接池的最大空闲时间
	int _connectionTimeout;            // 连接池的最大超时时间 

	queue<Connection*> _connectionQue; // MySQL 连接队列
	mutex _queueMutex;				   // 线程安全的互斥锁 
	atomic_int _connectionCnt;         // 原子类型，防止在多线程中 ++ 和 -- 操作导致出现线程安全问题
	condition_variable cv;             // 设置条件变量，用于连接生产和消费线程之间的通信
};
