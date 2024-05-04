#ifndef __MIN_HEAP_H__
#define __MIN_HEAP_H__

#include <netinet/in.h>
#include <time.h>
#include <iostream>
#include <set>
#include <functional>
#include <memory>

/**
 * 最小堆实现的定时器——时间堆
 * 
*/

using std::exception;

#define BUFFER_SIZE 64

class HeapTimer;

struct ClientData {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    HeapTimer* timer;
};

// 定时器类
class HeapTimer {
public:
    typedef std::shared_ptr<HeapTimer> ptr;

    HeapTimer(int delay) {      // delay 秒
        expire = time(0) + delay;
    }
    
    ~HeapTimer () {
        std::cout << "~HeapTimer()" << std::endl;
    }

public:
    time_t expire;                              // 定时器生效的绝对时间
    std::function<void(ClientData*)> cb_func;   // 定时器的回调函数
    ClientData* user_data;                      // 用户数据，回调函数参数

public:
    struct Comparator {
        bool operator()(const HeapTimer::ptr& lhs, const HeapTimer::ptr& rhs) const {
            if (!lhs && !rhs) {
                return false;
            } 
            if (!lhs) {
                return true;
            }
            if (!rhs) {
                return false;
            }
            if (lhs->expire < rhs->expire) {
                return true;
            }
            if (rhs->expire < lhs->expire) {
                return false;
            }
            return lhs.get() < rhs.get();
        }
    };
    
};

// 时间堆类
class TimeHeap {
public:
    typedef std::set<HeapTimer::ptr, HeapTimer::Comparator> HeapTimerArray;
    // 初始化一个大小为 cap 的空堆
    TimeHeap() : cur_size(0) {}
    
    // 用已有数组初始化堆
    TimeHeap(const HeapTimerArray& other) = delete;
    TimeHeap& operator=(const HeapTimerArray& other) = delete;


    ~TimeHeap() {
    }

    // 添加目标定时器
    void addTimer(HeapTimer::ptr timer) {
        if (!timer) {
            return;
        }
        array.insert(timer);
    }

    // 删除目标定时器 timer
    void delTimer(HeapTimer::ptr timer) {
        if (!timer) {
            return;
        }
        
        // 1 仅仅将目标定时器的回调函数置空，即所谓的延迟销毁。
        // 这将节省真正删除该定时器造成的开销，但是这样做容易导致堆数组膨胀
        timer->cb_func = nullptr;

        // 2 先查找再删除
        // auto it = array.find(timer);
        // if (it == array.end()) {
        //     return;
        // }
        // array.erase(it);
    }

    // 获得堆顶定时器
    HeapTimer::ptr top() const {
        if (array.empty()) {
            return nullptr;
        }
        return *array.begin();
    }

    // 删除堆顶的定时器
    void popTimer() {
        if (array.empty()) {
            return;
        }
        array.erase(array.begin());
    }

    // 心搏函数
    void tick() {
        auto it = array.begin();
        time_t now_time = time(0);  // 循环处理堆中到期的定时器
        while (it != array.end()) {
            // 堆顶定时器没有到期，退出循环
            if ((*it)->expire > now_time) {
                break;
            }
            // 否则执行堆顶定时器中的任务
            if ((*it)->cb_func) {
                (*it)->cb_func((*it)->user_data);
            }
            // 将堆顶元素删除，同时生成新的堆顶定时器
            popTimer();
            it = array.begin();
        }
    }

private:
    HeapTimerArray array;   // 堆数组
    int capacity;           // 堆数组的容量
    int cur_size;           // 堆数组当前包含元素的个数
};

#endif  // __MIN_HEAP_H__