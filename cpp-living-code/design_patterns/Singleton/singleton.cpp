#include <iostream>
#include <mutex>
#include <atomic>
using namespace std;

// 懒汉式单例模式
class LazySingleton
{
private:
    static LazySingleton* volatile m_instance;  // 使用 volatile 关键字防止 reorder
    static std::mutex m_mutex;

    LazySingleton() = default;
    LazySingleton(const LazySingleton& other) = delete;
    LazySingleton& operator=(const LazySingleton& other) = delete;

public:
    static LazySingleton* getInstance();
    
};
LazySingleton* volatile LazySingleton::m_instance = nullptr;

// 线程不安全的懒汉式单例模式
LazySingleton* LazySingleton::getInstance()
{
    if (m_instance == nullptr)
    {
        m_instance = new LazySingleton();
    }
    return m_instance;

}

LazySingleton* volatile LazySingleton::m_instance = nullptr;
std::mutex LazySingleton::m_mutex;

// 线程安全的懒汉式单例模式
LazySingleton* LazySingleton::getInstance() {
	// 如果没有第一个if也是线程安全的，但这样就会导致多个线程调用getInstance，每次都要加锁解锁
	if (m_instance == nullptr) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_instance == nullptr) { // 防止构造多次对象
			m_instance = new LazySingleton();
		}
	}
	return m_instance;
}


// 饿汉式单例模式
class HungrySingleton
{
private:
    static HungrySingleton* volatile m_instance;  // 使用 volatile 关键字防止 reorder

    HungrySingleton() = default;
    HungrySingleton(const HungrySingleton& other) = delete;
    HungrySingleton& operator=(const HungrySingleton& other) = delete;

public:
    static HungrySingleton* getInstance();
};

HungrySingleton* volatile HungrySingleton::m_instance = new HungrySingleton();

HungrySingleton* HungrySingleton::getInstance() {
	return m_instance;
}

// Meyers' Singleton
// 解决了普通单例模式全局变量初始化依赖
// C++ 只能保证在同一个文件中声明的 static 遍历初始化顺序和其遍历声明的顺序一致，但是不能保证不同文件中 static 遍历的初始化顺序
// C++11 保证了 static 成员初始化线程安全
class MeyersLazySingleton
{
private:
    MeyersLazySingleton() = default;
    MeyersLazySingleton(const MeyersLazySingleton& other) = delete;
    MeyersLazySingleton& operator=(const MeyersLazySingleton& other) = delete;

public:
    static MeyersLazySingleton& getInstance();
};
MeyersLazySingleton& MeyersLazySingleton::getInstance() {
    // 函数局部静态变量的初始化，在汇编指令上已经自动添加线程互斥的指令
    // 局部的static变量在编译后就已经存在于数据段了，但初始化操作是运行到该代码才会执行，也就是编译期间该对象并没有构造
    static MeyersLazySingleton instance;
	return instance;
}

int main(int argc, char const *argv[])
{
    
    return 0;
}
