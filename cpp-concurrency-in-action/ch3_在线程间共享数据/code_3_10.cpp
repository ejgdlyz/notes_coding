#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <climits>
#include <mutex>
using namespace std;


// 仅仅在访问共享数据期间才锁住互斥

class some_class
{};
class result_type
{};
std::mutex the_mutex;
some_class get_next_data_chunk();
result_type process(some_class sc);
void write_result(some_class sc, result_type r);

void get_and_process_data()
{
    std::unique_lock<std::mutex> my_lock(the_mutex);;
    some_class data_to_process = get_next_data_chunk();

    my_lock.unlock();

    result_type result = process(data_to_process);  // 假定调用 process 期间，互斥无需加锁

    my_lock.lock();  // 重写锁互斥，以写结果

    write_result(data_to_process, result);
}


// 代码清单 3.10 在比较运算中，每次只锁住一个互斥

class Y
{
private:
    int some_detail;
    mutable std::mutex m;
    int get_detail() const
    {
        std::lock_guard<std::mutex> lock_a(m);  // 加锁保护数据，再返回数据的副本
        return some_detail;
    }

public:
    Y(int sd) : some_detail(sd) {}
    friend bool operator==(Y const& lhs, Y const& rhs)
    {
        if (&lhs == &rhs)
            return true;
        
        int const lhs_value = lhs.get_detail();  // 一次只持有一个锁，排除了死锁的可能， 原来需要两个对象一起锁定
        int const rhs_value = rhs.get_detail();

        return lhs_value == rhs_value;
    }
};

// 这篡改了比较运算的原有语义
// 上述代码返回 true 的意义是：lhs.some_detail 在某个时刻的值等于 rhs.some_detail 在另一个时刻的值
// 在两次获取之间，它们的值可以任意变化。这篡改了比较运算的语义。
