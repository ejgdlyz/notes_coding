#include <iostream>
#include <memory>
#include <mutex>
using namespace std;

// 在初始化过程中保护共享数据

class some_resource
{
public:
    void do_something();
};

// 单线程情况
std::shared_ptr<some_resource> resource_ptr;
void foo()
{
    if (!resource_ptr)
    {
        resource_ptr.reset(new some_resource);
    }
    resource_ptr->do_something();
}


// 转换为多线程
std::mutex resource_mutex;
void foo()
{
    std::unique_lock<std::mutex> lk(resource_mutex);  // 此处，所有线程都被迫循序运行
    if (!resource_ptr)
    {
        resource_ptr.reset(new some_resource);  // 仅有初始化需要保护
    }

    lk.unlock();
    resource_ptr->do_something();
}

// 所有线程都必须在互斥上轮候，等待查验数据是否已经完成初始化
// 上述代码毫无必要地迫使多个线程循序运行 改进 ----> 饱受诟病的 双重检验锁定模式

void undefined_behaviour_with_double_checked_locking()
{
    
    if (!resource_ptr)
    {
        std::unique_lock<std::mutex> lk(resource_mutex);  
        if (!resource_ptr)
        {
            resource_ptr.reset(new some_resource);  
        }
    }
    resource_ptr->do_something();  // do_something 可能被两个线程执行了两次，而且每次执行的数据不同
}
// 可能引发恶性条件竞争
// 问题的根源是 当前线程在锁的保护范围外读取指针，而对方线程却可能先获取锁，顺利进入锁保护范围内执行写操作，读写没有同步，产生了条件竞争。


// 使用 std::call_once() 和 std::once_flag 
std::once_flag resource_flag;
void init_resource()
{
    resource_ptr.reset(new some_resource);
}

void fOO()
{
    std::call_once(resource_flag, init_resource);  // 初始化函数被准确地唯一一次调用
    resource_ptr->do_something();
}