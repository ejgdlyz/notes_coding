#include <iostream>
#include <chrono>
#include <algorithm>

#include <list>
#include <mutex>
using namespace std;

std::list<int> some_list;  // 全局 list
std::mutex some_mutex;  // 全局 mutex，保护 list

void add_to_list(int new_value)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find)
{
    std::lock_guard<std::mutex> guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find) != some_list.end();
}

int main(int argc, char* argv[])
{
    cout << "Main thread..." << endl;
    return 0;
}