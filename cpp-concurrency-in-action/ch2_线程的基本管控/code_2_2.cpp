#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

struct func
{
    int& i;
    func(int& i_) : i(i_) {}
    void operator() ()
    {

    }
};
void do_something_in_current_thread();

void f()
{
    int some_local_state = 0;
    func my_func(some_local_state);

    std::thread t(my_func);
    try
    {
        do_something_in_current_thread();
    }
    catch(...)
    {
        t.join();
        throw;
    }
    t.join();
}

// 更简洁有效的实现方式 利用 RAII 等待线程完结
class thread_guard
{
    std::thread& t;
public:
    explicit thread_guard(std::thread& t_) : t(t_) {}
    ~thread_guard()
    {
        if (t.joinable())  // 是否可汇合，join 只能汇合一次
        {
            t.join();  // 汇合，即 等待线程结束
        }
    }
    thread_guard(const thread_guard&) = delete;  // 禁止复制和赋值，防止生存期问题 比如 两次析构等
    thread_guard& operator=(const thread_guard&) = delete;
};

