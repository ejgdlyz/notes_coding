#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

class BigObject
{
public:
    BigObject()
    {
        cout << "默认构造函数" << endl;
    }
    BigObject(const BigObject& rhs)  
    {
        cout << "拷贝构造函数" << endl;
    }
    
    BigObject(BigObject&& rhs)
    {
        cout << "移动构造函数" << endl;
    }

    BigObject& operator=(const BigObject& rhs)
    {
        cout << "拷贝赋值" << endl;
        return *this;
    }
    BigObject& operator=(BigObject&& rhs)
    {
        cout << "移动赋值" << endl;
        return *this;
    }

    ~BigObject()
    {
        cout << "析构函数" << endl;
    }

};
void process_big_object(std::unique_ptr<BigObject> p)
{
    std::cout << "process_big_object..." << std::endl;

}
int main(int argc, char* argv[])
{
    // std::unique_ptr<BigObject> p(new BigObject);

    // std::thread t(process_big_object, std::move(p));
    // t.join();
    
    BigObject p;
    BigObject p1 = p;
    BigObject p2(p);
    BigObject p3(std::move(p1));
    
    return 0;
}