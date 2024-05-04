#include <iostream>
#include <thread>
#include <exception>
#include <memory>
#include <stack>
#include <mutex>

#include <queue>
#include <condition_variable>

using namespace std;

std::mutex m;
std::queue<int> data_queue;
std::condition_variable data_cond;

void data_preparation_thread()
{
    for (int i = 0; i <= 1000; ++i)
    {
        {
            std::lock_guard<std::mutex> lock(m);
            data_queue.push(i);
        }
        data_cond.notify_one();
    }      

}

void data_processing_thread()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(m);
        data_cond.wait(lock, [](){return !data_queue.empty();});

        int i = data_queue.front();
        data_queue.pop();
        lock.unlock();
        std::cout << i << std::endl;
        if (i == 1000)
        {
            break;
        }
    }
}
void f()
{
    std::thread t1{data_preparation_thread};
    std::thread t2{data_processing_thread};
    t1.join();
    t2.join();
}

int main(int argc, char* argv[])
{
    f();
    cout << "Main thread..." << endl;
    return 0;
}