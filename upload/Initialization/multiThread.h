#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include "header.h"
#include "platformThread.h"
#include "platformSocket.h"
#include "debug.h"

#define SIZE 512
#define THREAD_POOL_SIZE 28
#define TASK_QUEUE_SIZE 64

typedef struct
{
    char buf[SIZE];
    int len;
    struct sockaddr_in clientAddr;
    int clientAddrLen;
} Task;

extern my_mutex *queueMutex;
extern my_semaphore *queueNotEmpty;
extern my_semaphore *queueNotFull;
extern int taskHead, taskTail;
extern Task taskQueue[TASK_QUEUE_SIZE];

// 加任务，入队列
void addTask(Task *t);

void *workerThread(void *lpParam);

void initLockAndSemaphore();

#endif