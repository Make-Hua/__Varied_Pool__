
#include <iostream>
#include <chrono>
#include <thread>

#include "ThreadPool.h"

using uLong = unsigned long long;


class MyTask : public Task {
public:
    MyTask(int begin, int end)
        : begin_(begin), end_(end)
    {};

    Any run() {
        //std::cout << "tid:" << std::this_thread::get_id() << "begin()" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        uLong sum = 0;
        for (int i = begin_; i <= end_; ++i) {
            sum += i;
        }
        //std::cout << "tid:" << std::this_thread::get_id() << "end()" << std::endl;

        return sum;
    }

private:
    int begin_;
    int end_;
};

int main() {
    ThreadPool thPool;
    // 设置工作模式
    thPool.setMode(PoolMode::MODE_CACHED);

    // 开始启动线程池
    thPool.start(4);

    Result res1 = thPool.submitTask(std::make_shared<MyTask>(1, 100000000));
    Result res2 = thPool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    Result res3 = thPool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    
    thPool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    thPool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
    thPool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

    /*   线程通信
    由于调用 get() 函数时一定得是线程执行完 run 方法了
    int sum = res.get().cast_<int>(); */
    uLong sum1 = res1.get().cast_<uLong>();
    uLong sum2 = res2.get().cast_<uLong>();
    uLong sum3 = res3.get().cast_<uLong>();

    // Master - Slave 线程模型
    // Master 线程用来分解任务，如何给各个 Slave 线程分配任务
    // 等待 Slave 线程完成任务后， Master 线程将合并各个任务输出
    //std::cout << (sum1 + sum2 + sum3) << std::endl;

    

    /*thPool.submitTask(std::make_shared<MyTask>());
    thPool.submitTask(std::make_shared<MyTask>());
    thPool.submitTask(std::make_shared<MyTask>());
    thPool.submitTask(std::make_shared<MyTask>());*/

    getchar();

    return 0;
}
