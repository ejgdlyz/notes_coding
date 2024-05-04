#include <stdio.h>
#include <unistd.h>
#include <signal.h>

/*
int sigaction(int signo, const struct sigaction * act, struct sigaction * oldact);
    param1: 与signal相同，传递信号信息
    param2: param1对应的信号处理函数
    param3: 之前注册的信号处理函数指针

struct sigaction 
{
    void (*sa_handler)(int); // 处理函数的地址
    sigset_t sa_mask;       
    int sa_flags;
    
    // sa_mask 和 sa_flag 用于指定信号相关的选项和特性，在消灭僵尸进程时用不到, 取0即可
}

*/
void timeout(int sig)
{
    if (sig == SIGALRM)
    {
        puts("Time out!");
    }
    alarm(2);
}

int main(int argc, char const *argv[])
{
    int i;
    struct sigaction act;
    act.sa_handler = timeout;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGALRM, &act, 0);

    alarm(2);

    for ( i = 0; i < 3; ++i)
    {
        puts("wait...");
        sleep(100);
    }
    return 0;
}
