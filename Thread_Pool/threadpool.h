#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>


// ���������run ������Ҫ����ֵ���������� Task �أ�
// Any ��
class Any {
public:

	Any() = default;
	~Any() = default;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// Any �ں� unique_ptr�� ���޷�ʹ�� ��ֵ�������� �� ��ֵ����
	// ���� unique_ptr ��������ֵ�������� �� ��ֵ����
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;

	// ���Խ��������������͵�����
	template<typename T>
	Any(T data) : base_(std::make_unique<Derive<T>>(data)) {};

	// �� Any �����д洢�� data ��ȡ����
	template<typename T>
	T cast_() {
		// �� base ���ҵ� Derive����ȡ�� data
		// �漰�� ����ָ�� --->  ������ָ��   RTTI
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (nullptr == pd) {
			throw "type is unmatch!";
		}
		return pd->data_;
	}

private:
	// ����
	class Base {
	public:
		virtual ~Base() = default;
	};

	// ������
	template<typename T>
	class Derive : public Base {
	public:
		Derive(T data) : data_(data) {};
		T data_;
	};

private:
	std::unique_ptr<Base> base_;        // �����ָ��
};

// �ź�����
class Semaphore {
public:
	Semaphore(int limit = 0) : resLimit_(limit) {};
	~Semaphore() = default;

	void wait() {
		std::unique_lock<std::mutex> lock(mtx_);
		
		// �ȴ��ź�������Դ������Դ����
		cond_.wait(lock, [&]() -> bool { return resLimit_ > 0; });
		--resLimit_;
	}

	void post() {
		std::unique_lock<std::mutex> lock(mtx_);
		++resLimit_;
		cond_.notify_all();
	}

private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;

};

class Task;
// ʵ�ֽ�����
class Result {
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// setVal ,��ȡ����ִ�����ķ���ֵ���� any_;
	void setVal(Any any);

	// get ,�û��������������ȡtask�ķ���ֵ;
	Any get();

private:
	Any any_;          // �洢���񷵻�ֵ
	Semaphore sem_;    // �ź���
	std::shared_ptr<Task> task_; // ָ���Ӧ��ȡ����ֵ���������
	std::atomic_bool isValid_;   // 
};


/*
	����ģʽ:
		��  fixed   �̶��������߳�
		��  cached  �߳��������Զ�̬������
*/
// + class ʹ��ö��ʱ����������ĸ�������
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED,
};


// ����������
// �߳�������Դ��������ͻ����ͨ������ run ���������Զ���
class Task {
public:
	Task();
	~Task() = default;

	void exec();
	void setResult(Result* res);

	virtual Any run() = 0;
private:
	Result* result_;      // ��ָ����������Ҫǿ�� Task��
};


// �߳���
class Thread {
public:
	// ������������
	using ThreadFunc = std::function<void(int)>;

	Thread(ThreadFunc func);
	~Thread();

	// �����߳�
	void start();
	
	// ��ȡ�߳� ID
	int getId() const;

private:

	ThreadFunc func_;
	static int generateId_;
	int threadId_;               // �����߳� id
};


// �̳߳�����
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	// �����̳߳�
	void start(int initThreadSize = 4);

	// �����̳߳صĹ���ģʽ
	void setMode(PoolMode mode);

	// ���� Task �������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	// ���� Task �������������ֵ
	void setThreadSizeThreshHold(int threadsizehold);

	// �̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	// ����Ҫ �������� �� ��ֵ����
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;

private:
	// �����̺߳���
	void threadFunc(int threadid);

	// ��� Pool ����״̬
	bool checkRunningState() const;


private:
	
	// threads_(vector) -> ���� unique_ptr ����Ϊ����������ʱ��
	// �����Զ����������д��ָ����ָ��Ķ��ڴ�

	// �߳����
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;     // �߳��б�
	int initThreadSize_;										   // ��ʼ�߳�����
	std::atomic_int curThreadSize_;                                // ��¼��ǰ�̳߳���������vector.size() ��ʵ���Ե����߳�������������vector�����̰߳�ȫ�ģ�
	std::atomic_int idleThreadSize_;                               // ��¼�����̵߳�����  
 	int threadSizeThreshHold_;                                     // �߳��������ֵ

	// �������
	std::queue<std::shared_ptr<Task>> taskQue_;                    // �������
	std::atomic_int taskSize_;                                     // ��������
	int taskQueMaxThreshHold_;                                     // ��������������ֵ

	// �̰߳�ȫ & ͨ��
	std::mutex taskQueMtx_;                                        // ��֤������е��̰߳�ȫ
	std::condition_variable notFull_;                              // ������в�Ϊ��
	std::condition_variable notEmpty_;                             // ������в�Ϊ��

	// ����
	PoolMode poolMode_;                                            // ��ǰ�̳߳صĹ���ģʽ
	std::atomic_bool isPoolRunning_;                               // ��ʾ��ǰ�̳߳ص�����״̬
};




#endif
