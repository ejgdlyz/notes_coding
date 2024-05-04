#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <climits>
#include <mutex>
using namespace std;

// 代码清单 3.7 使用层级互斥预防死锁

class hierarchical_mutex;
hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int do_low_level_stuff();
int low_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void do_high_level_stuff(int some_param);
void high_level_func()
{
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    high_level_func();
}

void thread_a()
{
    high_level_func();  // 线程 a 正常运行
}

void do_other_stuff();
void other_stuff()
{
    high_level_func();
    do_other_stuff();
}

void thread_b()
{
    std::lock_guard<hierarchical_mutex> lk(other_mutex);  // 运行期出错。 先加锁层级编号 6000 的互斥，又加锁 10000 的互斥，违反了层级规则
    other_stuff();
}


// 代码清单 3.8 简单的层级互斥 实现

class hierarchical_mutex
{
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;
    unsigned long previous_hierarchy_value;

    // 线程专属的变量名 this_thread_hierarchy_value 表示当前线程的层级编号
    // 即当前线程最后一次加锁操作锁牵涉的层级编号, 初始化为 unsigned long 最大值
    // thread_local 修饰，每个线程都有自己的 this_thread_hierarchy_value 副本，该变量在两个线程上的值无关联
    static thread_local unsigned long this_thread_hierarchy_value;  

    void check_for_hierarchy_violation()
    {
        if (this_thread_hierarchy_value <= hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violated");
        }

    }

    // 记录当前线程的层级编号，将其保存为"上一次加锁的层级"
    void update_heirarchy_value()
    {
        previous_hierarchy_value = this_thread_hierarchy_value;
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    explicit hierarchical_mutex(unsigned long value) : hierarchy_value(value), previous_hierarchy_value(0)
    {}

    void lock()
    {
        check_for_hierarchy_violation();  // 验证
        internal_mutex.lock();  // 委托内部的互斥加锁
        update_heirarchy_value();  // 更新层级编号
    }

    void unlock()
    {
        if (this_thread_hierarchy_value != hierarchy_value)
        {
            throw std::logic_error("mutex hierarchy violated");
        }
        this_thread_hierarchy_value = previous_hierarchy_value;  // 线程的层级按保存的值复原
        internal_mutex.unlock();
    }

    bool try_lock()
    {
        check_for_hierarchy_violation();
        // 解锁某个层级的互斥，但是发现他不是最后一个被加锁的，抛出异常
        if (!internal_mutex.try_lock())   // try_lock() 与 lock() 工作原理相同，若零一线程已在目标互斥上持锁，则立即返回false，完全不等待
            return false;
        
        update_heirarchy_value();
        return true;
    }
};
thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);



