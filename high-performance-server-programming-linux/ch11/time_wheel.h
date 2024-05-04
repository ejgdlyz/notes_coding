#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <functional>
#include <iostream>

/**
 * 简单的时间轮代码实现
*/

#define BUFFER_SIZE 64

class TwTimer;

// 绑定 socket 和定时器
struct ClientData {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    TwTimer* timer;
};

// 定时器类
class TwTimer {
public:
    TwTimer(int rot, int ts) 
        : next(nullptr), prev(nullptr), rotation(rot), time_slot(ts) {}

public:
    int rotation;                               // 记录定时器在时间轮转多少圈后生效
    int time_slot;                              // 记录定时器属于时间轮上哪个槽（对应的链表）
    std::function<void(ClientData*)> cb_func;   // 定时器回调函数
    ClientData* user_data;                      // 客户数据, 回调函数使用
    TwTimer* next;                              // 下一个定时器
    TwTimer* prev;                              // 前一个定时器
};

class TimeWheel {
public:
    TimeWheel () : cur_slot(0) {
        for (int i = 0; i < N; ++i) {
            slots[i] = nullptr;  // 初始化每个槽头节点
        }
    }
    
    ~TimeWheel() {
        // 遍历每个槽，并销毁其中的定时器
        for (int i = 0; i < N; ++i) {
            TwTimer* tmp = slots[i];
            while (tmp) {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }

    // 根据定时值 timeout 创建一个定时器，并将其插入到合适的槽中
    TwTimer* addTimer(int timeout) {
        if (timeout < 0) {
            return nullptr;
        }
        int ticks = 0;
        // 根据待插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，并将该滴答数保存在 ticks 中
        // 如果待插入定时器的超时值小于时间轮的槽间隔 SI，则将 ticks 向上取整，否则 ticks 下取整为 timeout / SI
        if (timeout < SI) {
            ticks = 1;
        } else {
            ticks = timeout / SI;
        }

        // 计算待插入的定时器在时间轮转动多少圈后被触发
        int rotation = ticks / N;
        // 计算待插入的定时器应该被插入哪个槽中
        int ts = (cur_slot + (ticks % N)) % N;
        // 创建新的定时器，它在时间轮转动 rotation 圈之后被触发，且位于第 ts 个槽上
        TwTimer* timer = new TwTimer(rotation, ts);

        // 如果第 ts 个槽中尚无任何定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头节点
        if (!slots[ts]) {
            std::cout << "add timer, rotation is " << rotation << ", ts is " 
                    << ts << ", cur_slot is " << cur_slot << std::endl;
            slots[ts] = timer;
        } else {
            // 将定时器插入第 ts 个槽中
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    // 删除目标定时器 timer
    void delTimer(TwTimer* timer) {
        if (!timer) {
            return;
        }
        int ts = timer->time_slot;
        // slot[ts] 是目标定时器所在槽的头节点，如果目标定时器就是该头节点，则需要重置第 ts 个槽的头节点
        if (timer == slots[ts]) {
            // 待删除的为头节点
            slots[ts] = slots[ts]->next;
            if (slots[ts]) {
                slots[ts]->prev = nullptr;
            }
            delete timer;
        } else {
            timer->prev->next = timer->next;
            if (timer->next) {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    // SI 时间到后，调用该函数，时间轮向前滚动一个槽的间隔
    void tick() {
        TwTimer* tmp = slots[cur_slot];  // 取得时间轮上当前槽的头节点
        std::cout << "current slot is " << cur_slot << std::endl;
        while (tmp) {
            std::cout << "tick the timer once" << std::endl;
            if (tmp->rotation > 0) {
                // 如果定时器的 rotation 值 > 0，则它在这一轮不起作用
                --tmp->rotation;  // 更新生效轮数
                tmp = tmp->next;
            } else {
                tmp->cb_func(tmp->user_data);
                if (tmp == slots[cur_slot]) {
                    std::cout << "delete header in cur_slot" << std::endl;
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot]) {
                        slots[cur_slot]->prev = nullptr;
                    }
                    tmp = slots[cur_slot];
                } else {
                    tmp->prev->next = tmp->next;
                    if (tmp->next) {
                        tmp->next->prev = tmp->prev;
                    }
                    TwTimer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;  // 更新时间轮的当前槽，以反映时间轮的转动
    }
private:
    static const int N = 60;    // 时间轮上槽的数目
    static const int SI = 1;    // 每 1s 时间轮转一次，即槽间隔为 1s
    TwTimer* slots[N];          // 时间轮的槽。其中每个元素指向一个定时器无序链表
    int cur_slot;               // 时间轮的当前槽
};
#endif