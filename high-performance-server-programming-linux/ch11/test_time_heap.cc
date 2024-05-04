#include <unistd.h>
#include "time_heap.h"

static TimeHeap time_heap;

void test() {
    HeapTimer::ptr timer1(new HeapTimer(2));
    timer1->cb_func = [](ClientData* user_data){
        std::cout << "call back1" << std::endl;
    };
    HeapTimer::ptr timer2(new HeapTimer(5));
    timer2->cb_func = [](ClientData* user_data){
        std::cout << "call back2" << std::endl;
    };

    time_heap.addTimer(timer1);
    time_heap.addTimer(timer2);
    int cnt = 10;
    while (cnt--) {
        sleep(1);
        time_heap.tick();
    }
}

int main(int argc, char const *argv[]) {
    test();
    return 0;
}