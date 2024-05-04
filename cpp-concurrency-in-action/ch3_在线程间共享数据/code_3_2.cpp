#include <iostream>
#include <thread>
#include <string>
#include <mutex>

// 意外地向外传递引用，指向受保护的共享数据

class some_data
{
    int a;
    std::string b;
public:
    void do_something();
};

class data_wrapper
{
private:
    some_data data;
    std::mutex m;
public:
    template<typename Function>
    void process_data(Function func)
    {
        std::lock_guard<std::mutex> l(m);
        func(data);  // 向使用者提供的函数传递受保护的共享数据
    } 
};

some_data* unprotected;
void malicious_function(some_data& protected_data)
{
    unprotected = &protected_data;
}

data_wrapper x;

void foo()
{
    x.process_data(malicious_function);  // 传入恶意函数
    unprotected->do_something();  // 以无保护的方式访问本应该受保护的共享数据
}
