#include <iostream>
#include <thread>

#include <exception>
#include <memory>
#include <stack>
#include <mutex>

using namespace std;

struct empty_stack : std::exception
{
    const char* what() const throw()
    {
        return "Stack is empty.";
    }
};

template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    threadsafe_stack() { }
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);  // 源对象上锁住互斥
        data = other.m;   // 复制操作，这里不使用初始化列表，从而保证互斥的锁定会横跨整个复制过程
    }

    threadsafe_stack& operator=(const threadsafe_stack& ) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()  // 返回时发生拷贝构造
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty())  // 弹出前，检查栈是否为空
            throw empty_stack();

        const std::shared_ptr<T> res(std::make_shared<T>(data.top()));  // 改动栈容器前设置返回值
        data.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty())
            throw empty_stack();
        
        value = data.top();
        data.pop();
    }

    bool empty() const 
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

void add_to_stack(threadsafe_stack<int>& st)
{
    for(int i = 4; i < 10; ++i)
    {
        st.push(i);
        std::cout << "push:" << i << std::endl;
    }
}
void pop_from_stack(threadsafe_stack<int>& st)
{
    int t;
    for(int i = 0; i < 5; ++i)
    {
        try
        {
            st.pop(std::ref(t));
            // auto val = st.pop();
            // *val = 110;  // 输出 110
            std::cout << "pop:" << t << std::endl;
        }
        catch(empty_stack& e)
        {
            cout << e.what() << endl;
        }
        
    }
}
void f()
{
    threadsafe_stack<int> st;
    std::thread t1{add_to_stack, std::ref(st)};
    std::thread t2{pop_from_stack, std::ref(st)};
    t1.join();
    t2.join();
}

class A
{
    public:
    A ()
    {
        std::cout << "A 默认构造" << std::endl;
    }
    A(const A& b)
    {
        std::cout << "A 拷贝构造" << std::endl;
    }
};
void foo(A& a)
{

}
void f2()
{
    A a;
    // foo(a);
    std::thread t1{foo, std::ref(a)};
    std::thread t2{foo, std::ref(a)};
    t1.join();
    t2.join();
}
int main(int argc, char* argv[])
{
    // f();
    f2();
    cout << "Main thread..." << endl;
    return 0;
}