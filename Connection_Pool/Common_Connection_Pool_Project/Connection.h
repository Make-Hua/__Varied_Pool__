#pragma once
// ��־
#define LOG(str) \
	cout << __FILE__ << " : " << __LINE__ << " " << \
	__TIMESTAMP__ << " : " << str << endl;

#include<mysql.h>
#include<iostream>
#include<string>
#include<ctime>


using namespace std;

/*
	ʵ�����ݿ� ��ɾ�Ĳ�
*/

class Connection {
private:
	MYSQL* _conn; // ��ʾ��MySQL Server��һ������
	clock_t _alivetime; // ������к�Ĵ��ʱ��

public:
	// ��ʼ�����ݿ�����
	Connection();

	// �ͷ����ݿ�������Դ
	~Connection();

	// �������ݿ�
	bool connect(string ip,
		unsigned short port,
		string user,
		string password,
		string dbname);

	// ���²��� insert��delete��update
	bool update(string sql);

	// ��ѯ���� select
	MYSQL_RES* query(string sql);

	// �����ʱ��
	void refresAliveTime() { 
		_alivetime = clock(); 
	}

	// ���ʱ��
	clock_t getAliveTime() const {
		return clock() - _alivetime;
	}
};