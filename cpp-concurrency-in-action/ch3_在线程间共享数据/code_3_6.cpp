#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <mutex>
using namespace std;

// 运用 std::lock 和 std::lock_guard<> 类模板，进行内部数据的互换操作

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
        
        std::lock(lhs.m, rhs.m);  // 锁住两个互斥
        // adopt_lock 指明互斥已经被锁住，即互斥上有锁存在 
        // std::lock_guard 实例应该据此接收锁的归属权，不得在构造函数内在加锁
        std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);  
        std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);
        swap(lhs.some_detail, rhs.some_detail);

        // 无论是正常返回，还是异常退出，lock_guard 都保证互斥全部正确解锁
        // 若加锁涉及多个互斥，lock() 函数的语义是 全员共同成败，要么全部成功锁定，要么没获取任何锁并抛出异常
        // C++ 17 进一步提供了新的 RAII 类模板 std::scoped_lock<>，与lock_guard<>完全等价
        // scoped_lock 是可变参数模板，接收各种互斥型别作为类模板参数列表，还以多个互斥对象作为构造函数的参数列表
        
        // scoped_lock 重写 上述代码
        // std::scoped_lock guard(lhs.m, rhs.m);  // C++17具有 隐式模板推导  std::scoped_lock<std::mutex, std::mutex> guard(lhs.m, rhs.m)
        // swap(lhs.some_detail, rhs.some_detail);
    }
};