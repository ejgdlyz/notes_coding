#include <stdio.h>
#include <sys/uio.h>

/*
    writev函数使用
    struct iovec
    {
        void * iov_base;  // 缓冲地址
        size_t iov_len;  // 缓冲大小
    }
*/

int main(int argc, char * argv[])
{
    struct iovec vec[2];
    char buf1[] = "ABCEFG";
    char buf2[] = "1234567";
    int str_len;

    vec[0].iov_base = buf1;
    vec[0].iov_len = 3;
    vec[1].iov_base = buf2;
    vec[1].iov_len = 4;

    str_len = writev(1, vec, 2);  // 1 是标准输出文件描述符 

    puts("");
    printf("Write bytes: %d \n", str_len);
    return 0;
}