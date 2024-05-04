#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace std;

// 线程函数想要接受 非const 引用，需要使用 std::ref 进行包装，否则将编译错误
struct widget_id
{
    widget_id()
    {
        std::cout << "widget_id 构造" << std::endl;
    }
    widget_id(const widget_id& w)
    {
        std::cout << "widget_id 拷贝构造" << std::endl;
    }
    widget_id(widget_id&& w)
    {
        std::cout << "widget_id 移动构造" << std::endl;
    }
};
struct widget_data
{
    int data_ = 0;
    widget_data()
    {
        std::cout << "widget_data 构造" << std::endl;
    }
    widget_data(const widget_data& w)
    {
        std::cout << "widget_data 拷贝构造" << std::endl;
    }
    widget_data(widget_data&& w)
    {
        std::cout << "widget_data 移动构造" << std::endl;
    }
};

// update_data_for_widget 的第 2 个 参数想以引用的方式传入，但是 std::thread 的构造函数并不知情，它会直接复制我们提供的值
// 线程库的内部代码把参数的副本（复制的data对象，位于新线程的内部存储空间） 当成 move-only 类别，并以右值的形式传递。
// 最终 update_data_for_widget 收到一个右值作为参数，而接受类型为左值，编译失败
// 解决方法是 使用 std::ref() 加以包装
void update_data_for_widget(widget_id w, widget_data& data)
{
    data.data_ = 100;
}

void display_status(const widget_data& data)
{
    std::cout << "display_status: " << data.data_ << std::endl;
}


void oops(widget_id w)
{
    widget_data data;
    // std::thread t(update_data_for_widget, w, data);  // 传递的是 data 的副本
    std::thread t(update_data_for_widget, w, std::ref(data));  // 此时传递的是 data 的引用
    
    display_status(data);

    t.join();

    display_status(data);


}

void f()
{
    widget_id w;
    oops(w);

}
int main(int argc, char const *argv[])
{
    
    f();
    return 0;
}


/*
widget_id 构造
widget_id 拷贝构造

widget_data 构造

widget_id 拷贝构造
display_status: 0
widget_id 移动构造
display_status: 100  
*/
