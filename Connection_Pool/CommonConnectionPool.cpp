#include"CommonConnectionPool.h"
#include"Connection.h"

// 构造函数
ConnectionPool::ConnectionPool() {
	if (!loadConfigFile()) return;

	// 创建初始数量连接
	for (int i = 0; i < _initSize; ++i) {
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->refresAliveTime(); // 进入队列的起始时间
		_connectionQue.push(p);
		_connectionCnt++;
	}

	// 启动新线程， 作为连接（生产者）
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();

	// 启动新定时线程， 扫描超过maxIdletime的空闲连接
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

 //保证线程安全  &  单例模式
// Scott Meyer在《Effective C++》中提出了一种简洁的 singleton 写法
// 保证c++ 版本为11以后的即可
// 获取实例
ConnectionPool* ConnectionPool::getConnectionPool() {
	static ConnectionPool pool;      // 静态  在编译时期会自动 lock 和 unlock 
	return &pool;
}

// 加载配置项
bool ConnectionPool::loadConfigFile() {

	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr) {
		LOG("mysql.ini file is not exist!!!");
		return false;
	}

	// feof 是检测流上的文件结束符的函数，如果文件结束，则返回非0值，否则返回0
	while (!feof(pf)) {
		char line[1024] = { 0 };
		/*
			C 库函数 char *fgets(char *str, int n, FILE *stream) 
			从指定的流 stream 读取一行，并把它存储在 str 所指向的字符串内。
			当读取 (n-1) 个字符时，或者读取到换行符时，或者到达文件末尾时，
			它会停止，具体视情况而定
		*/
		fgets(line, 1024, pf);
		string str = line;
		int index = str.find('=', 0);   // 配置文件中注释尽量别有等号，不然报错

		if (index == -1) continue;      // 无效，为注释行

		int end_index = str.find('\n', index);
		string k = str.substr(0, index);
		string val = str.substr(index + 1, end_index - index - 1);

		if (k == "ip") _ip = val;
		else if (k == "port") _port = atoi(val.c_str());
		else if (k == "username") _username = val;
		else if (k == "password") _password = val;
		else if (k == "dbname") _dbname = val;
		else if (k == "initSize") _initSize = atoi(val.c_str());
		else if (k == "_maxSize") _maxSize = atoi(val.c_str());
		else if (k == "maxIdleTime") _maxIdleTime = atoi(val.c_str());
		else if (k == "maxConnectionTimeOut") _connectionTimeout = atoi(val.c_str());
	}
}

// 生产者线程
void ConnectionPool::produceConnectionTask() {
	while(true) {
		unique_lock<mutex> lock(_queueMutex);

		/*
			当队列中还有线程时，会一直在以下循环中等待，等消费者消费连接池
			当队列为空时，则会添加新的连接，前提是目前总连接要小于_maxSize
			以上为该函数乃至该线程所做的事情
		*/

		while (!_connectionQue.empty()) {          
			cv.wait(lock);        // 生产线程进入等待状态
		}
		if (_connectionCnt < _maxSize) {
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refresAliveTime(); // 进入队列的起始时间
			_connectionQue.push(p);
			_connectionCnt++;
		}

		// 连接池已经可以用了
		cv.notify_all();
	}
}

// // 给外部提供接口,从连接池获取连接
// 消费者线程
shared_ptr<Connection> ConnectionPool::getConnection() {
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty()) {
		// sleep
		if (cv_status::timeout == cv.wait_for(lock, chrono::microseconds(_connectionTimeout))) {
			if (_connectionQue.empty()) {
				LOG("获取连接超时.......");
				return nullptr;
			}
		}
	}
	

	/*
		 shared_ptr智能指针析构的时候，会之间delete掉（自动析构，智能指针的功能）
		 但是实际上当消费者用完Connection之后，需要把Connection归还
	*/

	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			// 由于是在消费者线程调用，所以我们需要考虑队列的线程安全问题，则加锁
			unique_lock<mutex> lock(_queueMutex);
			pcon->refresAliveTime(); // 进入队列的起始时间
			_connectionQue.push(pcon);
		});

	_connectionQue.pop();
	
	// 某消费者已消费结束，通知消费者
	cv.notify_all();

	return sp;
}

// 定时线程，扫描时间空闲连接，以便对Connection进行回收
void ConnectionPool::scannerConnectionTask() {
	while (true) {
		// 模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		// 扫描整个队列，释放多余连接（长时间存活但没调用）
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize) {
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000)) {
				_connectionQue.pop();
				_connectionCnt--;
				delete p;
			}
			else {
				break;     // 队列头部没超时，则后面肯定没超时，则直接跳出
			}
		}
	}
}