#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

void f()
{
    // 创建 2 个执行线程和 3 个 std::thread 实例 t1, t2, t3, 并将线程归属权在实例之间多次转移
    void some_func();
    void some_other_func();

    std::thread t1(some_func);         // 启动新线程并将其与 t1 关联. t1 关联着执行线程，sone_func 在其上运行
    std::thread t2 = std::move(t1);    // 将与 t1 关联的新线程归属权交给 t2

    // 启动另一新线程，与一个 std::thread 类型的临时对象关联。
    // 新线程的归属权随即转移给 t1. 临时变量，无需显示调用move
    t1 = std::thread(some_other_func);  

    std::thread t3;         // t3 使用默认方式构造，未关联任何线程
    t3 = std::move(t2);     // t2 关联的线程的归属权转移给 t3， t2 为具名变量，故需要 std::move

    // 经过这些转移， 此时 t1 与运行 some_other_func 的线程关联， t2 没有关联线程，t3 与运行 some_func 的线程关联

    t1 = std::move(t3);    // 该赋值操作会终止整个程序 
    // 在本次转移之时，t1 已经与运行 some_other_func 的线程关联。因此，最后一次转移，std::terminate() 被调用，终止整个程序。以保持一致性

}

// -------------------------------------------------------------------------
// 从函数内部返回 std::thread 对象
std::thread f(int)
{
    void some_func();
    return std::thread(some_func);  // 临时变量 右值
}

std::thread g()
{
    void some_other_func(int);
    std::thread t(some_other_func, 42);  // 临时变量 右值
    return t;
}

// 归属权转移到函数内部
void f(std::thread t);
void g(int)
{
    void some_func();
    f(std::thread(some_func));  // 临时变量 右值
    std::thread t(some_func);
    f(std::move(t));
}

int main(int argc, char const *argv[])
{
    /* code */
    return 0;
}
