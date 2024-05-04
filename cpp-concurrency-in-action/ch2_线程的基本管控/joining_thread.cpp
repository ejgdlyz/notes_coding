#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

class joining_thread
{
    std::thread t;
public:
    joining_thread() noexcept = default;

    template<typename Callable, typename ... Args>
    explicit joining_thread(Callable&& func, Args&& ... args) : t(std::forward<Callable>(func), std::forward<Args>(args)...)
    {}

    explicit joining_thread(std::thread t_) noexcept : t(std::move(t_))
    {}

    joining_thread(joining_thread&& other) noexcept : t(std::move(other.t))
    {
    }

    joining_thread& operator=(joining_thread&& other)  noexcept  // 执行完再复制
    {
        if (joinable())
            join();
        t = std::move(other.t);
        return *this;
    }

    joining_thread& operator=(std::thread other) noexcept
    {
        if (joinable())
            join();
        
        t = std::move(other);
        return *this;
    }

    ~joining_thread()  // 等待线程的完成
    {
        if (joinable())
            join();
    }

    void swap(joining_thread& other) noexcept
    {
        t.swap(other.t);
    }

    std::thread::id get_id() const noexcept
    {
        return t.get_id();
    }

    bool joinable() const noexcept
    {
        return t.joinable();
    }

    void join()
    {
        t.join();
    }

    void detach()
    {
        t.detach();
    }

    std::thread& as_thread() noexcept
    {
        return t;
    }

    const std::thread& as_thread() const noexcept
    {
        return t;
    }
};

void do_work(unsigned id)
{
    cout << id << endl;
}

void f()
{
    int hardware_threads = std::thread::hardware_concurrency();
    cout << "hardware_threads = " << hardware_threads << endl;
    std::vector<joining_thread> jthreads;
    for (int i = 0; i < hardware_threads; ++i)
    {
        jthreads.emplace_back(do_work, i);
    }

    // 等待所有线程完成后，才返回调用者

    // 等价于
    // std::vector<std::thread> threads;
    // for (int i = 0; i < 20; ++i)
    // {
    //     threads.emplace_back(do_work, i);
    // }
    // for(auto& entry : threads)
    // {
    //     entry.join();
    // }
}
int main(int argc, char* argv[])
{
    f();
    cout << "Main thread..." << endl;
    return 0;
}