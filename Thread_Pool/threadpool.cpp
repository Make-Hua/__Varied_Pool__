#include "threadpool.h"


const int TASK_MAX_THRESHHOLD = INT32_MAX;          // 线程数量（用来初始化线程池） 
const int THREAD_MAX_THRESHHOLD = 100;              // 线程最大数量
const int THREAD_MAX_IDLE_TIME = 60;                // 线程最大空闲时间 单位：s

ThreadPool::ThreadPool()
	: initThreadSize_(4)
	, taskSize_(0)
	, idleThreadSize_(0)
	, curThreadSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
{}


ThreadPool::~ThreadPool() {}

// 设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) {
		return;
	}
	poolMode_ = mode;
}

// 设置 Task 任务队列上线阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}
	taskQueMaxThreshHold_ = threshhold;
}

// 设置 Task 任务队列上线阈值
void ThreadPool::setThreadSizeThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}

	// 只有在 cache 模式下需要
	if (poolMode_ == PoolMode::MODE_CACHED) {
		threadSizeThreshHold_ = threshhold;
	}
}

// 提交任务到任务队列   用户调用该接口进行传入任务(生产任务)
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {

	//获取锁   (因为生产者和消费者都需要对任务队列进行操作，但 queue 本身并不线程安全，所以需要在临界区加锁)
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	// 线程通信    等待:当任务队列中任务数量 >= 阈值（taskQueMaxThreshHold_）则不能提交
	//while (taskQue_.size() == taskQueMaxThreshHold_) {
	//	notFull_.wait(lock);       // 进入等待状态   转变条件：首先需要等待通知，通知完后还要抢锁
	//}
	// 同理上
	bool CNT_ = notFull_.wait_for(lock,
		std::chrono::seconds(1),
		[&]() -> bool { return 	taskQue_.size() < (size_t)taskQueMaxThreshHold_; });
	if (!CNT_) {
		
		// 表示 notFull_ 等待了 1s 还是不满足条件(任务队列满了以后，任务还没有被线程消费)
		std::cout << "TaskQue is Full, submitTask fail." << std::endl;
		return Result(sp, false);
	}
	
	// 如果空余，则将 Task 放入 Que 中(当将Task 存入 Que 中时，则 Que 一定不为空，则通过线程通信[notEmpty_]进行通知)
	taskQue_.emplace(sp);
	++taskSize_;
	notEmpty_.notify_all();

	// cached 模式下，还需加根据任务数量和空闲线程数量，判断是否需要创建心新的线程出来？
	if (poolMode_ == PoolMode::MODE_CACHED 
		&& taskSize_ > idleThreadSize_ 
		&& curThreadSize_ < threadSizeThreshHold_) 
	{
		std::cout << ">>> create new thread" << std::endl;

		// 创建新线程
		std::function<void(int)> fs = std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1);
		auto ptr = std::make_unique<Thread>(fs);
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		// 启动线程
		threads_[threadId]->start();

		// 修改线程个数相关变量
		++curThreadSize_;
		++idleThreadSize_;
	}

	return Result(sp, true);
}

// 启动线程池
void ThreadPool::start(int initThreadSize) {
	// 设置状态
	isPoolRunning_ = true;
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	/* bind 的作用
	（1）将可调用对象与其参数一起绑定成一个仿函数。
	（2）将多元（参数个数为 n， n>1）可调用对象转成一元或者（ n-1）元可调用对象，即只绑定部分参数。
	*/
	// 创建线程对象
	// bind 为了将threadFunc作为一个线程的执行函数，
	// 并且确保在执行时能够访问到ThreadPool对象的成员变量和方法。
	// 
	// 下述用bind是因为：
	//		将当前 ThreadPool->this 对象与 threadFunc(线程函数) 绑定成一个新的函数对象
	//      这样子在将该函数对象给 Thread 类，通过组合的方式，让线程函数能在自己的 start
	//      函数中能直接用 ThreadPool->this 对象 和 ThreadPool类方法
	// 
	// 下述用 std::move 是因为：
	//		如果没有加 move ，将会发生拷贝操作。拷贝操作实际上试图复制 std::unique_ptr 
	//		对象的指针和所有权信息。但是unique_ptr 是一个独占所有权的智能指针，它的设计
	//		目的是确保在任何时候只有一个指针可以拥有资源的所有权，所以存在错误，则我们
	//		需要通过 move 进行所有权转移。
	// 

	for (int i = 0; i < initThreadSize_; ++i) {
		std::function<void(int)> fs = std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1);
		auto ptr = std::make_unique<Thread>(fs);
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}

	// 启动所有线程
	for (int i = 0; i < initThreadSize_; ++i) {

		// 执行一个线程函数
		threads_[i] -> start();

		// 空闲线程数量，刚创建的线程为空闲
		++idleThreadSize_;                    
	}


}

// 定义线程函数
void ThreadPool::threadFunc(int threadid) {

	// 获取起始时间，在 cache 模式下需要进行多创建线程的判断
	auto lastTime = std::chrono::high_resolution_clock().now();

	while (true) {

		std::shared_ptr<Task> task;
		// 该作用域主要是让 lock 出作用域后自动释放锁
		// 因为当前锁在执行 run 方法时并不需要抢占锁
		{
			//获取锁   (因为生产者和消费者都需要对任务队列进行操作，但 queue 本身并不线程安全，所以需要在临界区加锁)
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id() << "尝试获取任务..." << std::endl;

			// cached 模式下，可以已经有很多线程了，此时部分新建线程闲置时间超过60s
			// 超过 initThreadSize_ 数量的线程需要进行回收   time > 60s
			if (poolMode_ == PoolMode::MODE_CACHED) {

				// 每一秒需要返回一次时间，  ---》 怎么区分是超时返回 还是 因为执行任务返回
				while (taskQue_.size() == 0) {
					
					// 条件变量超时返回
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						// 当前时间
						auto now = std::chrono::high_resolution_clock().now();
						// 计算用时
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						

						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&& curThreadSize_ > initThreadSize_) {

							/*  回收线程：修改记录线程数量值 、 线程列表对应线程 pop 
								结束线程可以在线程函数中直接 return， 但是该如何从 vec 中 pop 出来对应的呢？？？
								threadid ---> thread ---> erase 即可 */
							// 根据索引删除此线程
							threads_.erase(threadid);

							--curThreadSize_;
							--idleThreadSize_;

							std::cout << ">>> threadid:" << std::this_thread::get_id() << "exit;" << std::endl;
							
							return;
						}
					} 
				}

			} else {
				// 等待线程通信的 notEmpty 条件
				notEmpty_.wait(lock, [&]() -> bool { return taskQue_.size() > 0; });
			}

			// 线程睡醒准备那 Task 执行任务时，则减少一个空闲线程
			--idleThreadSize_;

			std::cout << "tid:" << std::this_thread::get_id() << "获取任务成功..." << std::endl;

			// 等待结束后(此时非空) 则从任务队列取出一个任务来
			task = taskQue_.front();
			taskQue_.pop();
			--taskSize_;

			// 线程通信
			// notFull_  如果取出一个后 taskQue_ 中还有任务，则通知其他线程来消费
			// notEmpty_ 线程消费一个任务后，一定不满，则通知生产者进行生产
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			notFull_.notify_all();
		}

		// 当前线程负责执行的这个任务
		if (nullptr != task) {
			// task->run();
			task->exec();
		}

		// 任务执行完毕，增加一个空闲线程
		++idleThreadSize_;

		// 更新执行完 Task 的时间
		lastTime = std::chrono::high_resolution_clock().now();
	}
}

// 检查线程池状态
bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}



/*————————————————————————————————*/
/* 线程方法实现 */

int Thread::generateId_ = 0;

Thread::Thread(ThreadFunc func)
	: func_(func)
	, threadId_(generateId_++)
{}

Thread::~Thread() {}

void Thread::start() {

	// 创建线程对象（需要提供：线程函数或者函数对象,并可以同时指定线程的参数）
	std::thread t(func_, threadId_);
	t.detach();               // 分离线程  (由于t线程出了作用域就停止了,所以只能通过 bind)

}

// 获取线程 ID
int Thread::getId() const {
	return threadId_;
}


/*————————————————————————————————*/
/* Task方法实现 */

Task::Task()
	: result_(nullptr)
{}

void Task::exec() {
	if (nullptr != result_) {
		result_->setVal(run());
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}



/*————————————————————————————————*/
/* Result方法实现 */

Result::Result(std::shared_ptr<Task> task, bool isValid)
	: task_(task), isValid_(isValid) {
	task_ -> setResult(this);
}

Any Result::get() {             // 用户调用
	if (!isValid_) {
		return "";
	}

	// task 任务如果没有执行完，则会阻塞任务线程
	sem_.wait();

	return std::move(any_);
}

void Result::setVal(Any any) {  // 线程池中的线程函数中执行

	// 存储 task 的返回值
	this->any_ = std::move(any);

	sem_.post();

}




