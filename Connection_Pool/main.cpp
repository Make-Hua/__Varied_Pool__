#include<iostream>
#include"Connection.h"
#include"CommonConnectionPool.h"

using namespace std;

int main() {

	/*           单线程Test：1000    -->  1306ms / 1336ms / 1243ms              */
	/*  单线程Test_ConnPool：1000    -->  329ms / 230ms / 269ms                 */
	/*           单线程Test：5000    -->  5833ms / 5248ms / 6092ms              */
	/*  单线程Test_ConnPool：5000    -->  1174ms / 1354ms / 1322ms              */
	/*          单线程Test：10000    -->  10462ms / 10299ms / 9861ms            */
	/* 单线程Test_ConnPool：10000    -->  2413ms / 2215ms / 2364ms              */

	/*           多线程Test：1000    -->  369ms / 448ms / 533ms              */
	/*  多线程Test_ConnPool：1000    -->  167ms / 154ms / 157ms                 */
	/*           多线程Test：5000    -->  2576ms / 2753ms / 2666ms              */
	/*  多线程Test_ConnPool：5000    -->  529ms / 518ms / 550ms              */
	/*          多线程Test：10000    -->  4190ms / 5203ms / 4893ms            */
	/* 多线程Test_ConnPool：10000    -->  1064ms / 1038ms / 1045ms              */

	

	/*            Test 单线程
	clock_t begin = clock();

	ConnectionPool* cp = ConnectionPool::getConnectionPool();
	for (int i = 0; i < 5000; ++i) {
		Connection conn;
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
			"zhang_san", 20, "male");
		conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
		conn.update(sql);
		
		
		
		shared_ptr<Connection> sp = cp->getConnection();
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
			"zhang_san", 20, "male");
		sp->update(sql);

	}

	clock_t end = clock();
	cout << end - begin << "ms" << endl;
	*/
	clock_t begin = clock();

	/*
	ConnectionPool* cp = ConnectionPool::getConnectionPool();
	thread t1([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i) {
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});
	thread t2([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i) {
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});
	thread t3([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i) {
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});
	thread t4([]() {
		ConnectionPool* cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i) {
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});

	t1.join();  t2.join(); t3.join(); t4.join();
	*/
	/*
	Connection conn;
	conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	thread t1([]() {
		for (int i = 0; i < 2500; ++i) {
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
	});

	thread t2([]() {
		for (int i = 0; i < 2500; ++i) {
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
		});

	thread t3([]() {
		for (int i = 0; i < 2500; ++i) {
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
		});

	thread t4([]() {
		for (int i = 0; i < 2500; ++i) {
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang_san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
		});

	t1.join();  t2.join(); t3.join(); t4.join();
	*/
	clock_t end = clock();
	cout << end - begin << "ms" << endl;

	return 0;
}