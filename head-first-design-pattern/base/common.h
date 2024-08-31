#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

template<typename T>
void print(const T &value)
{
    std::cout << value << std::endl;
}

template<typename T, typename... Args>
void print(const T &first, const Args&... args)
{
    std::cout << first;
    print(args...);
}

template<typename... Args>
void println(const Args&... args)
{
    print(args...);
}
