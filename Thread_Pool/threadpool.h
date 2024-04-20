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


// 如果有需求，run 函数需要返回值，那如何设计 Task 呢？
// Any 类
class Any {
public:

	Any() = default;
	~Any() = default;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// Any 内涵 unique_ptr， 则无法使用 左值拷贝构造 和 赋值构造
	// 但是 unique_ptr 开放了右值拷贝构造 和 赋值构造
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;

	// 可以接收任意其他类型的数据
	template<typename T>
	Any(T data) : base_(std::make_unique<Derive<T>>(data)) {};

	// 将 Any 对象中存储的 data 提取出来
	template<typename T>
	T cast_() {
		// 将 base 中找到 Derive对象，取出 data
		// 涉及了 基类指针 --->  派生类指针   RTTI
		Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
		if (nullptr == pd) {
			throw "type is unmatch!";
		}
		return pd->data_;
	}

private:
	// 基类
	class Base {
	public:
		virtual ~Base() = default;
	};

	// 派生类
	template<typename T>
	class Derive : public Base {
	public:
		Derive(T data) : data_(data) {};
		T data_;
	};

private:
	std::unique_ptr<Base> base_;        // 基类的指针
};

// 信号量类
class Semaphore {
public:
	Semaphore(int limit = 0) : resLimit_(limit) {};
	~Semaphore() = default;

	void wait() {
		std::unique_lock<std::mutex> lock(mtx_);
		
		// 等待信号量有资源，无资源阻塞
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
// 实现接受类
class Result {
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// setVal ,获取任务执行完后的返回值存入 any_;
	void setVal(Any any);

	// get ,用户调用这个方法获取task的返回值;
	Any get();

private:
	Any any_;          // 存储任务返回值
	Semaphore sem_;    // 信号量
	std::shared_ptr<Task> task_; // 指向对应获取返回值的任务对象
	std::atomic_bool isValid_;   // 
};


/*
	两种模式:
		①  fixed   固定数量的线程
		②  cached  线程数量可以动态增长的
*/
// + class 使用枚举时需加上属于哪个作用域
enum class PoolMode {
	MODE_FIXED,
	MODE_CACHED,
};


// 任务抽象基类
// 线程任务可以创建该类型基类后通过重新 run 方法进行自定义
class Task {
public:
	Task();
	~Task() = default;

	void exec();
	void setResult(Result* res);

	virtual Any run() = 0;
private:
	Result* result_;      // 该指针生命周期要强于 Task的
};


// 线程类
class Thread {
public:
	// 函数对象类型
	using ThreadFunc = std::function<void(int)>;

	Thread(ThreadFunc func);
	~Thread();

	// 启动线程
	void start();
	
	// 获取线程 ID
	int getId() const;

private:

	ThreadFunc func_;
	static int generateId_;
	int threadId_;               // 保存线程 id
};


// 线程池类型
class ThreadPool {
public:
	ThreadPool();
	~ThreadPool();

	// 启动线程池
	void start(int initThreadSize = 4);

	// 设置线程池的工作模式
	void setMode(PoolMode mode);

	// 设置 Task 任务队列上线阈值
	void setTaskQueMaxThreshHold(int threshhold);

	// 设置 Task 任务队列上线阈值
	void setThreadSizeThreshHold(int threadsizehold);

	// 线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);

	// 不需要 拷贝构造 和 赋值构造
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;

private:
	// 定义线程函数
	void threadFunc(int threadid);

	// 检查 Pool 运行状态
	bool checkRunningState() const;


private:
	
	// threads_(vector) -> 中用 unique_ptr 是因为当结束任务时，
	// 可以自动析构容器中存放指针所指向的堆内存

	// 线程相关
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;     // 线程列表
	int initThreadSize_;										   // 初始线程数量
	std::atomic_int curThreadSize_;                                // 记录当前线程池总数量（vector.size() 其实可以等于线程总数量，但是vector不是线程安全的）
	std::atomic_int idleThreadSize_;                               // 记录空闲线程的数量  
 	int threadSizeThreshHold_;                                     // 线程数量最大值

	// 任务相关
	std::queue<std::shared_ptr<Task>> taskQue_;                    // 任务队列
	std::atomic_int taskSize_;                                     // 任务数量
	int taskQueMaxThreshHold_;                                     // 任务队列数量最大值

	// 线程安全 & 通信
	std::mutex taskQueMtx_;                                        // 保证任务队列的线程安全
	std::condition_variable notFull_;                              // 任务队列不为满
	std::condition_variable notEmpty_;                             // 任务队列不为空

	// 其他
	PoolMode poolMode_;                                            // 当前线程池的工作模式
	std::atomic_bool isPoolRunning_;                               // 表示当前线程池的启动状态
};




#endif
