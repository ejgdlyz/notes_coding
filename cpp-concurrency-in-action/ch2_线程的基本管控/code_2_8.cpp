#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

// 生成多个线程，并等待它们完成运行

void f()
{
    void do_work();
    std::vector<std::thread> threads;
    for(unsigned i = 0; i < 20; ++i)
    {
        threads.emplace_back(do_work, i);
    }

    for (auto& t : threads)
    {
        t.join();
    }
}

int main(int argc, char const *argv[])
{
    return 0;
}
