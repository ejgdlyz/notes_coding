#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;

using uL = unsigned long;

template<typename Iterator, typename T>
struct accumulate_block
{
    void operator() (Iterator first, Iterator last, T& result)
    {
        cout << "thread_id: " << std::this_thread::get_id() << endl;
        result =  std::accumulate(first, last, result);  // 前闭后开区间, 不包含 last
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    const uL length = std::distance(first, last);
    if (!length)
        return init;
    
    const uL min_per_thread = 25;
    const uL max_threads = (length + min_per_thread - 1) / min_per_thread;  // 线程的最大数量 = 元素总量 / 每个线程处理元素的最低限定量
    const uL hardware_threads = std::thread::hardware_concurrency();  // CPU 核心数量
    const uL num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

    const uL block_size = length / num_threads;  // 总量分块，计算各线程需要分担的数量
    
    std::vector<T> results(num_threads);  // 存放中间结果
    std::vector<std::thread> threads(num_threads - 1);  // 主线程算一个

    Iterator block_start = first;
    for(uL i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);  // 下一个开始，左闭右开
        threads[i] = std::thread(accumulate_block<Iterator, T>(), 
            block_start, block_end, std::ref(results[i]));  // 传入引用获取返回结果
        block_start = block_end;
    }

    accumulate_block<Iterator, T>()(block_start, last, results[num_threads - 1]);  // 发起所有线程后，主线程处理最后一块

    for(auto& entry: threads)
    {
        entry.join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}

void f()
{
    vector<uL> vec;
    uL sum = 0;
    for (uL i = 1; i <= 100000000; ++i)
    {
        sum += i;
        vec.push_back(i);
    }

    auto res = parallel_accumulate<vector<uL>::iterator, uL>(vec.begin(), vec.end(), 0);
    
    cout << "res = " << res << endl;
    cout << "sum = " << sum << endl;
    

}
int main(int argc, char* argv[])
{
    f();
    cout << "Main thread..." << endl;
    return 0;
}