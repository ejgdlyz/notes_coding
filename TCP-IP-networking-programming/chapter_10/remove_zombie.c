#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
    利用信号机制移除僵尸进程
*/

void read_childproc(int sig)
{
    int status;
    /*
    等待种植的目标子进程id，-1表示等待任意子进程终止，此时与wait相同
    子进程终止时传递的返回值（exit, return）将保存到该函数的参数所指的内存空间 status
    status包含其他信息，需要利用宏分离提取想要的信息
        WIFEXITED: 子进程终止时返回 true
        WEXITSTATUS: 返回子进程的返回值
    */
    pid_t id = waitpid(-1, &status, WNOHANG); 
    if (WIFEXITED(status))
    {
        printf("Removed proc id: %d \n", id);
        printf("Child send: %d \n", WEXITSTATUS(status));
    }
}

int main(int argc, char const *argv[])
{
    pid_t pid;
    // 注册sigchild信号对应的处理器
    struct sigaction act;
    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);

    pid = fork();
    if (pid == 0)  // 子进程执行区域
    {
        puts("Hi! I am child process");
        sleep(10);
        return 12;
    }
    else  // 父进程执行区域
    {
        printf("Child proc id: %d \n", pid);
        
        pid = fork();
        if (pid == 0)  // 零一子进程执行区域
        {
            puts("Hi! I am child process");
            sleep(10);
            exit(24);
        }
        else
        {
            int i;
            printf("child proc id: %d \n", pid);

            for (i = 0; i < 5; ++i)
            {
                puts("wait...");
                sleep(5);
            }
        }

    }
    return 0;
}


