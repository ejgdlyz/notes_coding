#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <climits>
#include <mutex>
using namespace std;

// 转移互斥归属权

void prepare_data();

std::unique_lock<std::mutex> get_lock()
{
    extern std::mutex some_mutex;  
    std::unique_lock<std::mutex> lk(some_mutex);  // 先锁定互斥
 
    prepare_data();  // 数据的前期准备

    return lk;  // 线程的归属权返回给调用者, 由于是局部变量，所以自动转移，编译器会调用移动构造函数
}

void do_something();
void process_data()
{
    std::unique_lock<std::mutex> lk(get_lock());  // 直接接受锁的转移

    do_something();
}

// 通道类 (gate way) 是一种利用锁转移的具体形式，锁的角色是其数据成员，用于保证只有正确加锁才能够访问受保护的数据，而不再充当函数的返回值。
// 所有的数据通过 通道类访问：访问数据前，先取得 通道类的实例(由函数返回，如 ger_lock())，再借他执行加锁操作，然后通过通道对象的成员函数才得访问数据
// 访问完成后，释放，其他线程得以重新访问数据。