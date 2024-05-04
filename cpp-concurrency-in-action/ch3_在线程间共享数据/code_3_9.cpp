#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <climits>
#include <mutex>
using namespace std;

// 代码清单 3.9 使用 std::lock() 和 std::unique_lock<> 类模板在对象间互换内部数据
// 重写代码清单 3.6

class some_big_object
{};
void swap(some_big_object& lhs, some_big_object& rhs);
class X
{
private:
    some_big_object some_detail;
    std::mutex m;
public:
    X(some_big_object const& sd): some_detail(sd) {}

    friend void swap(X& lhs, X& rhs)
    {
        if (&lhs == &rhs)
        {
            return;
        }
        
        std::unique_lock<std::mutex> lock_a(lhs.m, std::defer_lock);  // 实例 std::defer_lock 将互斥保留为无锁状态
        std::unique_lock<std::mutex> lock_b(rhs.m, std::defer_lock);

        std::lock(lock_a, lock_b);  // 这里才对互斥进行加锁
        
        swap(lhs.some_detail, rhs.some_detail);
    }
};

/*
 std::unique_lock 含有一个内部标志，随着函数的执行而更新，以表明关联的互斥目前是否正在被该类的实例占据。
 该标志保证析构函数正确调用unlock()。如果 std::unique_lock 占据着互斥，其析构函数必须调用 unlock(); 否则, 绝不能调用 unlock()
 此标志可通过调用 owns_lock() 查询。
 上述标志占据空间，所以  std::unique_lock 对象的 "体积" 往往大于  std::lock_guard 对象。该标志适时的嗯行或检查，存在轻微的性能损失。
*/