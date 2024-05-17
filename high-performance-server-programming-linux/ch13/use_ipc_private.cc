#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>

#include <iostream>

union semun {
    int val;
    struct semid_ds* buf;
    unsigned short int* array;
    struct seminfo* __buf;
};

/// op = -1 执行 P 操作，op = 1，执行 V 操作
void pv(int sem_id, int op) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}


int main(int argc, char const *argv[]) {
    
    int sem_id = semget(IPC_PRIVATE, 1, 0666);

    union semun sem_un;
    sem_un.val = 1;
    semctl(sem_id, 0, SETVAL, sem_un);

    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    } else if (pid == 0) {
        std::cout << "child try to get binary sem" << std::endl;

        /// 父子进程共享 IPC_PRIVATE 信号量的关键在于二者都可以操作该信号量的标识符
        pv(sem_id, -1);
        std::cout << "Child get the sem and would release it aftet 5 secs" << std::endl;
        sleep(5);
        pv(sem_id, 1);
        exit(0);
    } else {
        std::cout << "parent try to get binary sem" << std::endl;
        
        pv(sem_id, -1);
        std::cout << "parent get the sem and would release it aftet 5 secs" << std::endl;
        sleep(5);
        pv(sem_id, 1);
    }

    waitpid(pid, NULL, 0);
    semctl(sem_id, 0, IPC_RMID, sem_un);  // 删除信号量

    return 0;
}
