# 多进程编程

## 1 fork 系统调用
`pid_t fork(void);`
在父进程中返回子进程 pid，在子进程中返回 0。

## 2 exec 系列系统调用
在子进程中执行其他程序，即替换当前进程映像，需要使用如下 exec 系列函数之一：
```cpp
extern char** environ;

int execl(const char* path, const char* arg, ...);
int execlp(const char* file, const char* arg, ...);
int execle(const char* path, const char* arg, ..., char* const evnp[]);
int execv(const char* path, char* const argv[]);
int execvp(const char* file, char* const argv[]);
int execve(const char* path, char* const argv[], char* const envp[]);
```
- path: 可执行文件完整路径。
- file: 可以接收文件名，具体位置在环境变量 PATH 中搜索。
- arg： 接收可变参数，被传递给新程序的 main 函数。
- argv: 接收参数数组，被传递给新程序的 main 函数。
- envp: 设置新程序的环境变量，未设置则使用全局变量 environ 指定的环境变量。
- 出错返回 -1 并设置 errno。

## 3 处理僵尸进程
多进程程序中，父进程需要跟踪子进程的退出状态。所以，子进程结束，内核不会立即释放该进程的进程表表项，以让父进程查询。子进程退出，父进程读取其退出状态之前，子进程处于`僵尸态`。另一种子进程进入僵尸态的原因是：父进程结束或者异常终止，子进程继续运行，此时由 init 进程接管该子进程。

在父进程中调用下面的函数，以等待子进程的结束，并获取子进程的返回信息，避免僵尸进程的产生:
```cpp
pid_t wait(int* stat_loc);
pid_t waitpid(pid_t pid, int* stat_loc, int options);
```

wait() 函数将阻塞进程，直到该进程某个子进程结束运行，并返回结束运行的子进程 PID，子进程的退出状态存储于 stat_loc 指向的内存。通过宏查看 stat_loc 退出状态信息。

waitpid() 只等待由 pid 指定的子进程。pid=-1，同 wait()。options 可以控制该函数的行为，如 options = WHOHANG 时，该函数为非阻塞。若 pid 指向的进程没有结束或异常终止，waitpid() 返回 0；否则返回该退出子进程的 pid。

要在事件已经发生的情况下执行非阻塞调用才能提升效率。对于 waitpid()，最好在某个子进程退出之后再调用它。当一个子进程结束时，向其父进程发送 SIGCHILD 信号：
```cpp
static void handle_child(int sig) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &Stat, WHOHANG)) > 0) {
        // 对结束的子进程善后处理
    }
}
```

## 4 管道
管道可以在父子进程之间传递数据，利用的是 fork() 调用后两个管道文件描述符都保持打开。一对这些的文件描述符只能实现一个方向上的传输，一个关闭 pipefd[0]，另一个关闭 pipefd[1]。
```cpp
int pipe(pipefd[2]); 
```
调用成功：
- pipefd[0] 将会是管道的读端。
- pipefd[1] 将会是管道的写端。
要实现父子进程双向传输必须使用两个管道或者全双工的系统调用 `socketpair()`。

## 5 信号量
Linux 信号量的 API (`#include <sys/sem.h>`) 主要包含 3 个系统调用：semget、semop、semctl。

`int semget(key_t key, int num_sems, int sem_flags);`
- key: 键值，标识全局唯一的信号量集。
- nums_sems: 要创建/获取的信号量集中的信号量数目。创建信号量，必须指定该值；为 0 则表示获取已存在的信号量。
- sem_flags: 一组标志。低 9 位是信号量的权限。

semop() 用于改变信号量的值，即执行 P、V 操作：

`int semop(int sem_id, struct sembuf* sem_ops, size_t num_sem_ops);`

semctl() 允许调用者对信号量进行直接控制：
`int semctl(int sem_id, int sem_num, int command, ...);`
- sem_id: 即 semget() 返回的信号集标识符，
- sem_num: 指定被操作的信号量在信号集中的编号，
- command: 要执行的命令。

特殊键值 IPC_PRIVATE
semget() 调用者可以给其 key 参数传递一个特殊键值 IPC_PRIVATE，这样无论该信号是否已经存在，semget 都将创建一个新的信号量。其他进程，尤其子进程也可以访问该信号量。见 [代码](./use_ipc_private.cc)。

## 6 共享内存
共享内存是最高效的 IPC 机制，其不涉及进程之间的任何数据传输；但是必须使用同步手段来控制不同进程对共享内存的访问，以免产生竞态条件。共享内存相关的 API 都在 sys/shm.h 中，包括：shmget、shmat、shmdt 和 shmctl。

### shmget
`int shmget(key_t key, size_t size, int shmflg);` 
- shmget() 用于创建一段新的共享内存，或者获取一段已经存在的共享内存。成功返回一个正整数值，标识共享内存。
- key: 键值，用来标识一段全局唯一的共享内存。
- size: 指定共享内存的大小，单位字节。创建新内存，必须指定; size = 0，则获取已存在的内存。
- shmflg: 类似 semget() 的 sem_flags。并支持两个额外的标志
    - SHM_HUGETLB, 类似 mmap 的 MAP_HUGETLB, 系统将使用 “大页面”来为共享内存分配空间。
    - SHM_NORESERVE, 类似 mmap 的 MAP_NORESERVE 标志，不为共享内存保留交换分区（swap 空间）。当物理内存空间不足时，对该共享内存执行写操作将触发 SIGSEGV 信号。
- shmget() 创建共享内存时，该段内存所有字节被初始化为 0，与之关联的内核数据结构 shmid_ds 也被创建并初始化。
```cpp
struct shmid_ds {
    struct ipc_perm shm_perml;  /// 共享内存的操作权限
    size_t shm_segsz;           /// 共享内存大小，单位字节
    __time_t shm_atime;         /// 对这段内存最后一次调用 shmat 时间
    __time_t shm_dtime;         /// 对这段内存最后一次调用 shmadt 时间
    __time_t shm_ctime;         /// 对这段内存最后一次调用 shmctl 时间
    __pid_t shm_cpid;           /// 创建者的 PID
    __pid_t shm_lpid;           /// 最后一次执行 shmat 或 shmdt 操作的进程 PID
    shmatt_t shm_nattach;       /// 目前关联到此共享内存的进程数量
    /// 填充字段省略...
};
```

### shmat 和 shmdt 
共享内存被创建/获取之后，不能立即被访问，而是需要将其关联到进程的地址空间中。使用完共享内存后，也需要将其从地址空间中分离。这分别由如下两个系统调用实现：

`void* shmat(int shm_id, const void* shm_addr, int shmflg);`

`int shmdt(const void* shm_addr);`

- shm_id: shmget() 返回的共享内存标识符。
- shm_addr: 指定将共享内存关联到进程的哪块地址空间，受 shmflg 参数的可选标志 SHM_RND 影响：
    - shm_addr = NULL, 则被关联的地址由操作系统选择。推荐做法，保证可移植性。
    - shm_addr 非空，并设置了 SHM_RND 标志，则关联的地址是 `shm_addr - (shm_addr % SHMLBA)`。SHMLBA 即 段低端边界地址倍数，等于内存页面大小。SHM_RND 即圆整数，将共享内存被关联的地址向下圆整到离 shm_addr 最近的 SHMLBA 的整数倍地址处。
    - SHM_RDONLY: 进程仅能读取共享内存中的内容，没有指定，则可以同时读写。
    - SHM_REMAP: 如果地址 shmaddr 已经被关联到一段共享内存上，则重新关联。
    - SHM_EXEC: 指定对共享内存的执行权限，和读权限一样。
- shmat() 成功将修改内核数据结构 shmid_ds 的部分字段。
    - shm_nattach + 1；
    - shm_lpid 设置为调用进程的 PID；
    - shm_atime 设置为当前时间。
- shmdt() 将关联到 shm_addr 处的共享内存从进程中分离。shmdt() 成功返回 0，失败返回 -1 设置 errno。并且成功将修改内核数据结构 shmid_ds 的部分字段：
    - shm_nattach - 1;
    - shm_lpid 设置为当前调用进程的 pid；
    - shm_dtime 设置为当前的时间。

### shmctl 
用于控制共享内存的某些属性。
`int shmctl(int shm_id, int command, struct shmid_ds* buf);`
- shm_id: 共享内存标识符。
- command: 指定要执行的命令。支持的命令见表 13-3。


### 共享内存的 POSIX 方法
mmap() 函数利用它的 `MAP_ANONYMOUS` 标志可以实现父子进程之间的匿名内存共享。通过打开同一个文件，mmap 也可以实现无关进程之间的内存共享。Linux 提供另外一种利用 mmap 在无关进程之间共享内存的方式。此方式无需任何文件的支持，但需要使用如下函数创建或打开一个 POSIX 共享内存对象：
```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
int shm_open(const char* name, int oflag, mode_t mode);
```
- name: 指定要创建/打开的共享内存对象，应该使用 “/somename” 格式，以 “/” 开始，后接多个字符，且都不为 “/”；以 '\0' 结尾，长度不超过 NAME_MAX(255)。
- oflag: 指定创建方式。下列标志一个或多个按位或：
    - O_RDONLY: 以只读方式打开共享内存对象；
    - O_RDWR:  以可读、可写方式打开共享内存对象；
    - O_CREAT: 如果共享内存对象不存在，则创建；此时 mode 的低 9 位指向该共享内存对象的访问权限。共享内存被创建的时候，其初始长度为 0.
    - o_EXCL: 和 O_CREAT 一起使用。如果 name 指向的共享内存对象已经存在，则 shm_open() 返回错误，否则就创建一个新的共享内存对象；
    - o_TRUNC: 如果共享内存对象已经存在，将其截断，使其长度为 0。

shm_open 成功返回一个文件描述符，该描述符可用于后续的 mmap 调用，从而将共享内存关联到调用进程。由 shm_open 创建的共享内存对象使用完后也需要删除：

`int shm_unlink(const char* name);`
该函数将 name 指向的共享内存对象标记为等待删除。当所有使用该共享内存对象的进程都是用 unmmap 将其从进程分离之后，系统将销毁这个共享内存对象所占据的资源。

使用上述的 POSIX 共享内存函数，需要链接 -lrt。

### 共享内存实例
将聊天室改为多进程服务器，一个子进程处理一个客户连接。同时，将所有的客户 socket 连接的读缓冲区设计为一块共享内存，见[代码](./chatroom_server.cc)。