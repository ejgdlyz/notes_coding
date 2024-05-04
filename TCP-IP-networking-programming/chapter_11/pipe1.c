#include <stdio.h>
#include <unistd.h>

/*
    管道例子
*/

#define BUF_SIZE 30

int main(int argc, char const *argv[])
{
    int fds[2];
    char str[] = "who are you?";
    char buf[BUF_SIZE];
    pid_t pid;

    pipe(fds);
    pid = fork();

    if (pid == 0)  // 子进程 向管道传输字符串
    {
        write(fds[1], str, sizeof(str));
    }
    else
    {
        read(fds[0], buf, BUF_SIZE);
        puts(buf);
    }
    
    return 0;
}
