#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
/*
    当 scoped_thread 对象离开作用域前 确保线程已完结
    等待期间 main 线程可以继续执行
*/
class scoped_thread
{
    std::thread t_;
public:
    scoped_thread(std::thread t): t_(std::move(t))
    {
        cout << "scoped_thread" << endl;
        if (!t_.joinable())  // 如果 t_ 不可会合 直接抛异常  
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread()
    {
        t_.join();  // 构造函数判断，此处无需再判断
    }
    scoped_thread(const scoped_thread&) = delete;
    scoped_thread& operator=(scoped_thread const &) = delete;

};
void func()
{
    cout << "func " << endl;
}
int main(int argc, char* argv[])
{
    int local_state = 0;
    scoped_thread st{std::thread(func)};
    cout << "Main thread..." << endl;
    return 0;
}