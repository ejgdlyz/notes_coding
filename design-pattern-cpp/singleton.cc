#include <iostream>

struct Game 
{};

/// 懒汉模式：作为函数局部变量
template<class T, class X, int N>
T& GetInstance() 
{
    static T instance;
    // static thread_local T game;  /// 线程中的单例
    return instance;
}

template<class T, class X, int N>
T* GetInstancePtr() 
{
    static T instance;
    // static thread_local T game;  /// 线程中的单例
    return &instance;
}

template <class T>
class CSingeleton
{
public:
    static T& instance() 
    {
        static T s_instance;
        return s_instance;
    }

    // static T* instancePtr() 
    // {
    //     static T s_instance;
    //     return &s_instance;
    // }
};
class X : public CSingeleton<X> {};

int main(int argc, char const *argv[])
{
    return 0;
}
