#include <iostream>
#include <mutex>
#include <atomic>
#include <thread>
#include <string>

using namespace std;

bool flag = true;
int idx = 0;
std::mutex mutex_;
std::string str = "abcdefghi";
int length = str.size();

void test01()
{
    while (idx < length)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (flag)
            {
                if (idx < length)
                {
                    std::cout << "thread 1: " << str[idx++] << endl;
                    flag = !flag;
                }
            }
        }
    }
}
void test02()
{
    while (idx < length)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!flag)
            {
                if (idx < length)
                {
                    std::cout << "thread 2: " << str[idx++] << endl;
                    flag = !flag;
                }
                
            }
        }
    }
    
}

int main(int argc, char const *argv[])
{
    std::thread t1(test01);
    std::thread t2(test02);
    t1.join();
    t2.join();
    return 0;
}
