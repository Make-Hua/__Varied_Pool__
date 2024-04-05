#pragma once
/*
	ʵ�����ӳع���ģ��
	���ӳ���
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
	// ��ȡ���ӳض���ʵ��
	// ���ӳ�ֻ��Ҫһ��������ģʽ���
	static ConnectionPool* getConnectionPool();

	// ���ⲿ�ṩ�ӿ�,�����ӳػ�ȡ����
	shared_ptr<Connection> getConnection();


private:
	ConnectionPool();
	bool loadConfigFile();             // ���������ļ� mysql.ini
	void produceConnectionTask();      // �������������ӵģ������������һ���߳�
	void scannerConnectionTask();      // �����¶�ʱ�̣߳� ɨ�賬��maxIdletime�Ŀ�������

	string _ip;                        // MySQL ��Ӧ ip
	unsigned short _port;              // �˿ں� : 3306
	string _username;                  // MySQL ��Ӧ �û���
	string _password;                  // MySQL ��Ӧ ����
	string _dbname;                    // MySQL ��Ӧ ����
	int _initSize;                     // ���ӳصĳ�ʼ������
	int _maxSize;                      // ���ӳص����������
	int _maxIdleTime;                  // ���ӳص�������ʱ��
	int _connectionTimeout;            // ���ӳص����ʱʱ�� 

	queue<Connection*> _connectionQue; // MySQL ���Ӷ���
	mutex _queueMutex;				   // �̰߳�ȫ�Ļ����� 
	atomic_int _connectionCnt;         // ԭ�����ͣ���ֹ�ڶ��߳��� ++ �� -- �������³����̰߳�ȫ����
	condition_variable cv;             // ���������������������������������߳�֮���ͨ��
};
