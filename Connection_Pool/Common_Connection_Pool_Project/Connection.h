#pragma once
// 日志
#define LOG(str) \
	cout << __FILE__ << " : " << __LINE__ << " " << \
	__TIMESTAMP__ << " : " << str << endl;

#include<mysql.h>
#include<iostream>
#include<string>
#include<ctime>


using namespace std;

/*
	实现数据库 增删改查
*/

class Connection {
private:
	MYSQL* _conn; // 表示和MySQL Server的一条连接
	clock_t _alivetime; // 进入队列后的存活时间

public:
	// 初始化数据库连接
	Connection();

	// 释放数据库连接资源
	~Connection();

	// 连接数据库
	bool connect(string ip,
		unsigned short port,
		string user,
		string password,
		string dbname);

	// 更新操作 insert、delete、update
	bool update(string sql);

	// 查询操作 select
	MYSQL_RES* query(string sql);

	// 入队列时间
	void refresAliveTime() { 
		_alivetime = clock(); 
	}

	// 存活时间
	clock_t getAliveTime() const {
		return clock() - _alivetime;
	}
};