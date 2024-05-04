#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>

/*
    select 测试
    监测控制台输入
*/

#define BUF_SIZE 30

int main(int argc, char * argv[])
{
    fd_set reads, temps;
    int result, str_len;
    char buf[BUF_SIZE];
    struct timeval timeout;

    // 宏初始化 fd_set 变量
    FD_ZERO(&reads);
    FD_SET(0, &reads);  // 0 是标准输入, 将文件描述符0对应的位 置1

    while(1)
    {
        temps = reads; // 复位temps，调用selec后，除发生变化的文件描述符外，剩下的所有位将初始化为0
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        result = select(1, &temps, 0, 0, &timeout);  // 控制台有输入，则返回 >0 数，否则，如果超时，返回 0
        if (result == -1)
        {
            puts("select() error!");
            break;
        }
        else if(result == 0)
        {
            puts("Time-out!");
        }
        else
        {
            if (FD_ISSET(0, &temps))  // 验证发生变化的文件描述符是否为标准输入
            {
                str_len = read(0, buf, BUF_SIZE);
                buf[str_len] = 0;
                printf("message from console: %s", buf);
            }
        }
    }
    return 0;
}