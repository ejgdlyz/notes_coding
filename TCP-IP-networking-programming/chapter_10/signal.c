#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void timeout(int sig)
{
    if (sig == SIGALRM)
    {
        puts("Time out!");
    }
    alarm(2);  // 每个2s 插死你哼sgnalrm信号
}

void keycontrol(int sig)
{
    if (sig == SIGINT)
    {
        puts("CTRL + C pressed!");
    }
}

int main(int argc, char const *argv[])
{
    // void (*signal(int sigo, void (*func)(int)))(int);
    // signal的参数和返回值都是函数指针，
    /*
    两个参数 
        int sig：一个整数类型的参数，通常用于表示信号编号。
        void (*func)(int)：一个函数指针，指向一个接受整数参数且无返回值的函数。
    返回值
        函数指针 void (*)(int)，一个指向接受整数参数且无返回值的函数的函数指针。
    */
   
    int i;
    signal(SIGALRM, timeout);  // 注册 sigalrm 信号 和 控制器
    signal(SIGINT, keycontrol);  // 注册 sigint 信号 和 控制器
    alarm(2);

    for ( i = 0; i < 3; ++i)
    {
        puts("wait...");
        sleep(100);
    }
    // 大约5分钟后终止程序，但实际执行时只需不到10s
    // 发生信号时,为了调用方信号处理器，将唤醒由于调用sleep函数而进入阻塞状态的进程，即使未到sleep函数规定的时间
    // 一旦唤醒，就不会再进入睡眠状态
    return 0;
}
