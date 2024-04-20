#include "threadpool.h"


const int TASK_MAX_THRESHHOLD = INT32_MAX;          // �߳�������������ʼ���̳߳أ� 
const int THREAD_MAX_THRESHHOLD = 100;              // �߳��������
const int THREAD_MAX_IDLE_TIME = 60;                // �߳�������ʱ�� ��λ��s

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

// �����̳߳صĹ���ģʽ
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) {
		return;
	}
	poolMode_ = mode;
}

// ���� Task �������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}
	taskQueMaxThreshHold_ = threshhold;
}

// ���� Task �������������ֵ
void ThreadPool::setThreadSizeThreshHold(int threshhold) {
	if (checkRunningState()) {
		return;
	}

	// ֻ���� cache ģʽ����Ҫ
	if (poolMode_ == PoolMode::MODE_CACHED) {
		threadSizeThreshHold_ = threshhold;
	}
}

// �ύ�����������   �û����øýӿڽ��д�������(��������)
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {

	//��ȡ��   (��Ϊ�����ߺ������߶���Ҫ��������н��в������� queue �������̰߳�ȫ��������Ҫ���ٽ�������)
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	// �߳�ͨ��    �ȴ�:������������������� >= ��ֵ��taskQueMaxThreshHold_�������ύ
	//while (taskQue_.size() == taskQueMaxThreshHold_) {
	//	notFull_.wait(lock);       // ����ȴ�״̬   ת��������������Ҫ�ȴ�֪ͨ��֪ͨ���Ҫ����
	//}
	// ͬ����
	bool CNT_ = notFull_.wait_for(lock,
		std::chrono::seconds(1),
		[&]() -> bool { return 	taskQue_.size() < (size_t)taskQueMaxThreshHold_; });
	if (!CNT_) {
		
		// ��ʾ notFull_ �ȴ��� 1s ���ǲ���������(������������Ժ�����û�б��߳�����)
		std::cout << "TaskQue is Full, submitTask fail." << std::endl;
		return Result(sp, false);
	}
	
	// ������࣬�� Task ���� Que ��(����Task ���� Que ��ʱ���� Que һ����Ϊ�գ���ͨ���߳�ͨ��[notEmpty_]����֪ͨ)
	taskQue_.emplace(sp);
	++taskSize_;
	notEmpty_.notify_all();

	// cached ģʽ�£�����Ӹ������������Ϳ����߳��������ж��Ƿ���Ҫ�������µ��̳߳�����
	if (poolMode_ == PoolMode::MODE_CACHED 
		&& taskSize_ > idleThreadSize_ 
		&& curThreadSize_ < threadSizeThreshHold_) 
	{
		std::cout << ">>> create new thread" << std::endl;

		// �������߳�
		std::function<void(int)> fs = std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1);
		auto ptr = std::make_unique<Thread>(fs);
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

		// �����߳�
		threads_[threadId]->start();

		// �޸��̸߳�����ر���
		++curThreadSize_;
		++idleThreadSize_;
	}

	return Result(sp, true);
}

// �����̳߳�
void ThreadPool::start(int initThreadSize) {
	// ����״̬
	isPoolRunning_ = true;
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	/* bind ������
	��1�����ɵ��ö����������һ��󶨳�һ���º�����
	��2������Ԫ����������Ϊ n�� n>1���ɵ��ö���ת��һԪ���ߣ� n-1��Ԫ�ɵ��ö��󣬼�ֻ�󶨲��ֲ�����
	*/
	// �����̶߳���
	// bind Ϊ�˽�threadFunc��Ϊһ���̵߳�ִ�к�����
	// ����ȷ����ִ��ʱ�ܹ����ʵ�ThreadPool����ĳ�Ա�����ͷ�����
	// 
	// ������bind����Ϊ��
	//		����ǰ ThreadPool->this ������ threadFunc(�̺߳���) �󶨳�һ���µĺ�������
	//      �������ڽ��ú�������� Thread �࣬ͨ����ϵķ�ʽ�����̺߳��������Լ��� start
	//      ��������ֱ���� ThreadPool->this ���� �� ThreadPool�෽��
	// 
	// ������ std::move ����Ϊ��
	//		���û�м� move �����ᷢ��������������������ʵ������ͼ���� std::unique_ptr 
	//		�����ָ�������Ȩ��Ϣ������unique_ptr ��һ����ռ����Ȩ������ָ�룬�������
	//		Ŀ����ȷ�����κ�ʱ��ֻ��һ��ָ�����ӵ����Դ������Ȩ�����Դ��ڴ���������
	//		��Ҫͨ�� move ��������Ȩת�ơ�
	// 

	for (int i = 0; i < initThreadSize_; ++i) {
		std::function<void(int)> fs = std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1);
		auto ptr = std::make_unique<Thread>(fs);
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
	}

	// ���������߳�
	for (int i = 0; i < initThreadSize_; ++i) {

		// ִ��һ���̺߳���
		threads_[i] -> start();

		// �����߳��������մ������߳�Ϊ����
		++idleThreadSize_;                    
	}


}

// �����̺߳���
void ThreadPool::threadFunc(int threadid) {

	// ��ȡ��ʼʱ�䣬�� cache ģʽ����Ҫ���жഴ���̵߳��ж�
	auto lastTime = std::chrono::high_resolution_clock().now();

	while (true) {

		std::shared_ptr<Task> task;
		// ����������Ҫ���� lock ����������Զ��ͷ���
		// ��Ϊ��ǰ����ִ�� run ����ʱ������Ҫ��ռ��
		{
			//��ȡ��   (��Ϊ�����ߺ������߶���Ҫ��������н��в������� queue �������̰߳�ȫ��������Ҫ���ٽ�������)
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid:" << std::this_thread::get_id() << "���Ի�ȡ����..." << std::endl;

			// cached ģʽ�£������Ѿ��кܶ��߳��ˣ���ʱ�����½��߳�����ʱ�䳬��60s
			// ���� initThreadSize_ �������߳���Ҫ���л���   time > 60s
			if (poolMode_ == PoolMode::MODE_CACHED) {

				// ÿһ����Ҫ����һ��ʱ�䣬  ---�� ��ô�����ǳ�ʱ���� ���� ��Ϊִ�����񷵻�
				while (taskQue_.size() == 0) {
					
					// ����������ʱ����
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						// ��ǰʱ��
						auto now = std::chrono::high_resolution_clock().now();
						// ������ʱ
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						

						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&& curThreadSize_ > initThreadSize_) {

							/*  �����̣߳��޸ļ�¼�߳�����ֵ �� �߳��б��Ӧ�߳� pop 
								�����߳̿������̺߳�����ֱ�� return�� ���Ǹ���δ� vec �� pop ������Ӧ���أ�����
								threadid ---> thread ---> erase ���� */
							// ��������ɾ�����߳�
							threads_.erase(threadid);

							--curThreadSize_;
							--idleThreadSize_;

							std::cout << ">>> threadid:" << std::this_thread::get_id() << "exit;" << std::endl;
							
							return;
						}
					} 
				}

			} else {
				// �ȴ��߳�ͨ�ŵ� notEmpty ����
				notEmpty_.wait(lock, [&]() -> bool { return taskQue_.size() > 0; });
			}

			// �߳�˯��׼���� Task ִ������ʱ�������һ�������߳�
			--idleThreadSize_;

			std::cout << "tid:" << std::this_thread::get_id() << "��ȡ����ɹ�..." << std::endl;

			// �ȴ�������(��ʱ�ǿ�) ����������ȡ��һ��������
			task = taskQue_.front();
			taskQue_.pop();
			--taskSize_;

			// �߳�ͨ��
			// notFull_  ���ȡ��һ���� taskQue_ �л���������֪ͨ�����߳�������
			// notEmpty_ �߳�����һ�������һ����������֪ͨ�����߽�������
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}
			notFull_.notify_all();
		}

		// ��ǰ�̸߳���ִ�е��������
		if (nullptr != task) {
			// task->run();
			task->exec();
		}

		// ����ִ����ϣ�����һ�������߳�
		++idleThreadSize_;

		// ����ִ���� Task ��ʱ��
		lastTime = std::chrono::high_resolution_clock().now();
	}
}

// ����̳߳�״̬
bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}



/*����������������������������������������������������������������*/
/* �̷߳���ʵ�� */

int Thread::generateId_ = 0;

Thread::Thread(ThreadFunc func)
	: func_(func)
	, threadId_(generateId_++)
{}

Thread::~Thread() {}

void Thread::start() {

	// �����̶߳�����Ҫ�ṩ���̺߳������ߺ�������,������ͬʱָ���̵߳Ĳ�����
	std::thread t(func_, threadId_);
	t.detach();               // �����߳�  (����t�̳߳����������ֹͣ��,����ֻ��ͨ�� bind)

}

// ��ȡ�߳� ID
int Thread::getId() const {
	return threadId_;
}


/*����������������������������������������������������������������*/
/* Task����ʵ�� */

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



/*����������������������������������������������������������������*/
/* Result����ʵ�� */

Result::Result(std::shared_ptr<Task> task, bool isValid)
	: task_(task), isValid_(isValid) {
	task_ -> setResult(this);
}

Any Result::get() {             // �û�����
	if (!isValid_) {
		return "";
	}

	// task �������û��ִ���꣬������������߳�
	sem_.wait();

	return std::move(any_);
}

void Result::setVal(Any any) {  // �̳߳��е��̺߳�����ִ��

	// �洢 task �ķ���ֵ
	this->any_ = std::move(any);

	sem_.post();

}




