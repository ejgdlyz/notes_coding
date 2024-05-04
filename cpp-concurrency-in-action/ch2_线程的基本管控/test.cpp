#include <iostream>
#include <thread>
#include <chrono>

void func()
{
    std::cout << "Hello concurrency world!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

}
int main(int argc, char* argv[])
{

    std::thread t(func);
    t.join();
    
    std::cout << "main thread..." << std::endl;
    return 0;
}