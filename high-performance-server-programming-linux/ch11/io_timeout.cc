#include <time.h>
#include <sys/epoll.h>
#include <iostream>

#define TIMEOUT 5000

int timeout = TIMEOUT;
time_t start = time(0);
time_t end = time(0);
#define MAX_EVENT_NUMBER 1024

void main() {
    // ...
    int epollfd = epoll_create(64);
    epoll_event events[MAX_EVENT_NUMBER];
    // ...
    while (1) {
        std::cout << "the timeout is now " << timeout << " mil-seconds" << std::endl;
        start = time(0);
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, timeout);  // ms 
        if (number < 0) {
            std::cout << "epoll failure" << std::endl;
            break;
        }
        // 如果 epoll_wait 返回 0，说明超时时间已到，便可以处理超时任务并重置定时时间
        if (number == 0) {
            timeout = TIMEOUT;
            continue;
        }

        end = time(0);
        // 如果 epoll_wait 的返回值 > 0，则本次 epoll_wait 调用持续时间是 (end - start) * 1000 ms
        // 需要将定时时间 timeout 减去这段时间，以获得下次 epoll_wait 调用的超时参数
        timeout -= (end - start) * 1000;
        // 重新计算之后的 timeout 可能为 0，这说明本次 epoll_wait 返回不仅有文件描述符就绪，而且其超时时间也刚好过期，
        // 此时也需要处理定时任务，并重置定时时间
        if (timeout <= 0) {
            timeout = TIMEOUT;
        }
        // handle connections
    }
}
