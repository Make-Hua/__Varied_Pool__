#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define THREADPOOL_INIT_COUNT        20
#define TASK_INIT_SIZE               1000


// 添加链表
#define LIST_INSERT(item, list) do {              \
	item -> prev = NULL;					      \
	item -> next = list;					      \
	if (NULL != (list)) (list) -> prev = item;    \
	(list) = item;							      \
} while (0)

// 删除链表
#define LIST_REMOVE(item, list) do {                              	\
	if (NULL != item -> prev) item -> prev -> next = item -> next;	\
	if (NULL != item -> next) item -> next -> prev = item -> prev;	\
	if (list == item) list = item -> next;							\
	item -> prev = item -> next = NULL;                             \
} while (0)




// 任务队列
struct nTask {
	void (*task_func)(struct nTask *task);
	void *user_data;
	
	struct nTask *prev;                // list
	struct nTask *next;                // list
};

// 执行队列
struct nWorker {
	pthread_t threadid;
	struct nManager *manager;       // 需要操作管理组件结构体，则该结构体内需要有一个 nManager
	int terminate;

	struct nWorker *prev;         // list
	struct nWorker *next;         // list
};

// ThreadPool
typedef struct nManager {
	struct nTask *tasks;          // 任务队列 对象
	struct nWorker *workers;      // ...

	pthread_mutex_t mutex;        // 互斥量
	pthread_cond_t cond;          // 信号量
} ThreadPool;


static void *nThreadPoolCallback(void *arg) {
	
    // 分配一个工作线程结构体的内存空间，并将传入的参数 arg 转换为 struct nWorker 指针类型
    struct nWorker *worker = (struct nWorker *)arg;

	//printf("nThreadPoolCallback\n");
	
    // 线程仅在线程池结束时需要销毁，则线程需一直进入死循环
    while (1) {
		
        // 加锁，后面涉及临界资源
        pthread_mutex_lock(&worker->manager->mutex);
		
        // 当任务列表为空时，工作线程进入等待状态，等待 条件变量 的通知
        while (NULL == worker->manager->tasks) {
            if (worker->terminate) break;
			
            // 等待条件变量的通知，同时释放互斥锁
            pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex);
        }
		
        // 如果工作线程被要求终止，则跳出循环
        // 解锁互斥锁，退出循环
        if (worker->terminate) {
            pthread_mutex_unlock(&worker->manager->mutex);
            break;
        }

        // 获取任务列表中的第一个任务，并将其从任务列表中移除
        struct nTask *task = worker->manager->tasks;
        LIST_REMOVE(task, worker->manager->tasks);
		//printf("LIST_REMOVE\n");

        // 解锁互斥锁，允许其他线程访问任务列表
        pthread_mutex_unlock(&worker->manager->mutex);

        // 执行任务函数，并传入任务数据作为参数
        task->task_func(task);
    }

    // 释放工作线程结构体的内存空间
    free(worker);
}


int nThreadPoolCreate(ThreadPool *pool, int numWorkers) {

    // 特判 防止用户传入数据有误
    // 如果传入的线程池指针为空，则返回错误代码 -1
    // 如果传入的工作线程数量小于 1，则将其设置为 1，确保至少有一个工作线程
    if (NULL == pool) return -1; 
    if (numWorkers < 1) numWorkers = 1; 

	// 任务队列初始化时候为空
	memset(pool, 0, sizeof(ThreadPool));

    // 使用 PTHREAD_COND_INITIALIZER 宏初始化一个条件变量
    // 将初始化后的条件变量复制到线程池中的条件变量成员中
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER; 
    memcpy(&pool -> cond, &blank_cond, sizeof(pthread_cond_t)); 

	//printf("nThreadPoolCreate\n");
	
    // 使用 pthread_mutex_init 函数初始化线程池中的互斥锁
    pthread_mutex_init(&pool -> mutex, NULL); 

	// 循环创建指定数量的工作线程
    int i = 0;
    for (i = 0; i < numWorkers; ++i) { 
		
		// 分配一个工作线程结构体的内存空间
        struct nWorker *worker = (struct nWorker *)malloc(sizeof(struct nWorker)); 

		// 如果分配内存失败
        if (NULL == worker) { 
            perror("malloc error!");
            return -2;
        }

		 // 将工作线程结构体的内存清零
        memset(worker, 0, sizeof(struct nWorker));
        worker -> manager = pool;

        // 创建工作线程
        // 调用 pthread_create 函数创建线程，并指定回调函数和参数
        int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallback, worker); 
        if (ret) {
            perror("malloc error!");
			 // 异常退出时记得释放之前分配的工作线程结构体内存
            free(worker);
            return -3;
        }
		
		 // 将创建的工作线程插入到线程池的工作线程列表中
        LIST_INSERT(worker, pool->workers);
		//printf("LIST_INSERT\n");
    }
        
    return 0; // 返回成功代码 0
}



int nThreadPoolDestory(ThreadPool *pool, int nWorker) {

    // 声明一个工作线程指针
    struct nWorker *worker = NULL;

    // 遍历线程池中的所有工作线程
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        // 标记当前工作线程为终止状态
        worker->terminate;
    }

    // 加锁，防止其他线程同时访问线程池的条件变量和互斥锁
    pthread_mutex_lock(&pool->mutex);

    // 广播条件变量，通知所有等待的线程
    pthread_cond_broadcast(&pool->cond);

    // 解锁互斥锁，允许其他线程访问线程池的条件变量和互斥锁
    pthread_mutex_unlock(&pool->mutex);

    // 将线程池的工作线程列表和任务列表置为空
    pool->workers = NULL;
    pool->tasks = NULL;

    // 返回成功代码 0
    return 0;
}


int nThreadPoolPushTask(ThreadPool *pool, struct nTask *task) {
    // 加锁，防止其他线程同时修改任务列表
    pthread_mutex_lock(&pool->mutex);

    // 将任务插入到线程池的任务列表中
    LIST_INSERT(task, pool->tasks);
	//printf("LIST_INSERT\n");

    // 发送信号，通知等待的线程有新任务可执行
    pthread_cond_signal(&pool->cond);

    // 解锁互斥锁，允许其他线程访问任务列表
    pthread_mutex_unlock(&pool->mutex);

	
}


void task_entry(struct nTask *task) {

	// struct nTask *task = (struct nTask*)task;
	int idx = *(int *)task -> user_data;

	printf("indx %d \n", idx);

	free(task -> user_data);
	free(task);
}



int main() {

	ThreadPool pool;

	nThreadPoolCreate(&pool, THREADPOOL_INIT_COUNT);
	// printf("nThreadPoolCreate runOK！\n");

	int i = 0;
	for (i = 0; i < TASK_INIT_SIZE; ++i) {

	    // 创建任务
		struct nTask *task = (struct nTask*)malloc(sizeof(struct nTask));
		if (NULL == task) {
			perror("malloc");
			exit(1);
		}

		// 将任务置为空
		memset(task, 0, sizeof(struct nTask));

		// 任务函数给该指针
		task -> task_func = task_entry;
		task -> user_data = malloc(sizeof(int));
		*(int *)task -> user_data = i;

		// 提交任务
		nThreadPoolPushTask(&pool, task);     
		
	}

	getchar();
	return 0;
}











