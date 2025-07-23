#include "multiThread.h"
#include "platformSocket.h"

my_mutex *queueMutex;
my_mutex *ID_list_Mutex;
my_mutex *log_Mutex;
my_mutex *hash_table_Mutex;
my_semaphore *queueNotEmpty;
my_semaphore *queueNotFull;
int taskHead = 0, taskTail = 0;  // 全局变量，因为需要多线程共享
Task taskQueue[TASK_QUEUE_SIZE]; // 全局变量，因为需要多线程共享

void addTask(Task *t)
{
    my_waitSemaphore(queueNotFull);
    my_lockMutex(queueMutex);
    taskQueue[taskTail] = *t;
    taskTail = (taskTail + 1) % TASK_QUEUE_SIZE;
    my_unlockMutex(queueMutex);
    my_postSemaphore(queueNotEmpty);
}

static int getTask(Task *t)
{
    my_waitSemaphore(queueNotEmpty);
    my_lockMutex(queueMutex);
    *t = taskQueue[taskHead];
    taskHead = (taskHead + 1) % TASK_QUEUE_SIZE;
    my_unlockMutex(queueMutex);
    my_postSemaphore(queueNotFull);
    return 1;
}

void *workerThread(void *lpParam)
{
    void (*handler)(Task *) = (void (*)(Task *))lpParam;

    while (1)
    {
        Task t;
        getTask(&t);
        debug_print2("Receive %d bytes from %s:%d\n", t.len,
                     inet_ntoa(t.clientAddr.sin_addr), ntohs(t.clientAddr.sin_port));
        debug_dns_message_hex(t.buf, t.len);

        handler(&t);
    }
    return NULL;
}

void initLockAndSemaphore()
{
    // 多线程操作使用的三个句柄，互斥锁和两个信号量，两个信号量用于防止CPU忙等待
    queueMutex = my_createMutex();
    ID_list_Mutex = my_createMutex();
    log_Mutex = my_createMutex();
    hash_table_Mutex = my_createMutex();
    queueNotEmpty = my_createSemaphore(0, TASK_QUEUE_SIZE);
    queueNotFull = my_createSemaphore(TASK_QUEUE_SIZE, TASK_QUEUE_SIZE);
}