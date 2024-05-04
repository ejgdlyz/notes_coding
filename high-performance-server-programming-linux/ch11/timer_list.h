#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>
#include <iostream>
#include <memory>

/**
 *  基于升序双向链表的定时器
 * 效率：添加定时器的时间复杂度为 O(n)，删除定时器为 O(1), 执行定时器任务时间复杂度为 O(1)
*/

#define BUFFER_SIZE 64

class UtilTimer;

// 用户数据结构：客户端 socket 地址、socket fd、读缓存和定时器
struct ClientData {
    typedef std::shared_ptr<ClientData> ptr;

    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    UtilTimer* timer;
};

// 定时器类
class UtilTimer {
public:
    UtilTimer() : prev(nullptr), next(nullptr) {}

    time_t expire;                              // 任务超时时间，绝对时间
    std::function<void(ClientData*)> cb_func;   // 任务回调函数
    ClientData* user_data;                      // 回调函数处理的客户数据，由定时器的执行者传递给回调函数，即回调函数的参数
    UtilTimer* prev;
    UtilTimer* next;
};

// 定时器链表: 升序双向链表，且1带有头节点和尾节点
class SortedTimerLst {
public:
    SortedTimerLst() : head(nullptr), tail(nullptr) {}
    // 链表销毁时，删除其中所有的定时器
    ~SortedTimerLst() {
        UtilTimer* tmp = head;
        while (tmp) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    // 将目标定时器 timer 添加到链表中
    void addTimer(UtilTimer* timer) {
        if (!timer) {
            return;
        }
        if (!head) {
            head = tail = timer;
            return;
        }
        // 如果 timer 的超时时间小于当前链表中所有定时器的超时时间，则插入头部，作为新的头节点
        // 否则，就调用重载的 addTimer() 将其插入链表中合适的位置，以保持链表的升序
        if (timer->expire < head->expire) {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        addTimer(timer, head);
    }

    // 当某个定时任务发生变化，调整对应的定时器在链表中的位置
    // 此函数只考虑被调整的定时器的超时时间延长的情况，即该定时器只往尾部移动
    void adjustTimer(UtilTimer* timer) {
        if (!timer) {
            return;
        }

        UtilTimer* next_timer = timer->next;
        // 如果被调整的目标定时器 timer 处在链表尾部，或者该定时器新的超时值仍然小于下一个定义其的超时值，不用调整
        if (!next_timer || (timer->expire < next_timer->expire)) {
            return;
        }

        // 如果 timer 是链表的头节点，则从链表中取出 timer 并重新插入链表
        if (timer == head) {
            head = head->next;
            head->prev = nullptr;
            timer->next = nullptr;
            addTimer(timer, head);
        } else {
            // timer 不是链表头节点，则将该定时器从链表取出，将其插入原来位置之后的部分链表中
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            addTimer(timer, head);
        }
    }

    // 将目标定时器 timer 从链表中删除
    void delTimer(UtilTimer* timer) {
        if (!timer) {
            return;
        }
        if (timer == head && timer == tail) {
            // 链表中只有一个定时器且为目标定时器
            delete timer;
            head = nullptr;
            tail = nullptr;
            return;
        }
        // 链表有两个以上定时器，且 timer 为链表头节点
        if (timer == head) {
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }
        // timer 为链表尾节点
        if (timer == tail) {
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }
        
        // timer 位于链表中间，串联其前后的定时器，然后将其删除
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    //核心函数： SIGALRM 信号每次被触发就在其信号处理函数（如果统一事件源，则是主函数）
    // 中执行一次 tick 函数，以处理链表上到期的任务
    // tick() 每隔一段固定的时间就执行一次，以检测并处理到期的任务
    void tick() {
        if (!head) {
            return;
        }
        std::cout << "timer tick" << std::endl;
        time_t now_time = time(0);  // 获取系统当前时间
        UtilTimer* cur = head;
        // 从头节点开始依次处理每个定时器，直到遇到未到期的定时器
        while (cur) {
            // 每个定时器都是用绝对时间作为超时值，可以把定时器的超时值和系统当前时间进行比较以判断定时器是否到期
            if (now_time < cur->expire) {
                break;
            }
            // 调用定时器回调函数，执行定时任务
            cur->cb_func(cur->user_data);
            // 执行完毕定时器中的定时任务之后，将其从链表中删除，并重置链表的头节点
            head = cur->next;
            if (head) {
                head->prev = nullptr;
            }
            delete cur;
            cur = head;
        }
    }

private:
    // 重载的辅助函数，该函数将目标定时器 timer 添加到节点 lst_head 之后的部分链表中
    void addTimer(UtilTimer* timer, UtilTimer* lst_head) {
        UtilTimer* prev = lst_head;
        UtilTimer* next = prev->next;

        // 遍历 lst_head 之后的部分链表，直到找到一个超时时间 > timer 超时时间的节点，将 timer 插入到该节点之前
        while (next) {
            if (timer->expire < next->expire) {
                prev->next = timer;
                timer->next = next;
                next->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = next;
            next = next->next;
        }
        // 遍历完 lst_head 节点之后的部分链表，仍未找到超时时间 > timer 超时时间的节点，则直接插入尾部
        if (!next) {
            prev->next = timer;
            timer->next = nullptr;
            timer->prev = prev;
            tail = timer;
        }
    }

private: 
    UtilTimer* head;
    UtilTimer* tail;
};

#endif