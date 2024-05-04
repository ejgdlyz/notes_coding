#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

// 当前线程的函数已返回，新线程却仍能访问其局部变量

void do_something(const int& i);
struct func
{
    int& i;
    func(int& i_) : i(i_) {}
    void operator() ()
    {
        for(unsigned j = 0; j < 1000000; j++)
        {
            do_something(i);  // 可能访问悬空引用
        }
    }
};

void oops()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach();
}  // 新线程可能仍在运行，但是主线程的函数却已结束

// 解决方法时将线程完全自含，将数据复制到新线程的内部，而不是共享数据。
// 避免以下做法：意图在函数中创建线程，并让线程访问函数的局部变量
// 另一种解决方法时汇合新线程