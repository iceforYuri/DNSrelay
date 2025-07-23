#include "platformThread.h"
#include <stdlib.h>

#ifdef _WIN32

my_mutex* my_createMutex()
{
    my_mutex* m = (my_mutex*)malloc(sizeof(my_mutex));
    if (m) {
        *m = CreateMutex(NULL, FALSE, NULL);//安全属性，初始是否拥有，名字
    }
    return m;
}
void my_destroyMutex(my_mutex* m)
{
    if (m) {
        CloseHandle(*m);
        free(m);
    }
}
void my_lockMutex(my_mutex* m) {
    WaitForSingleObject(*m, INFINITE);
}
void my_unlockMutex(my_mutex* m) {
    ReleaseMutex(*m);
}

my_semaphore* my_createSemaphore(unsigned int initialValue, unsigned int maxValue) {
    my_semaphore* s = (my_semaphore*)malloc(sizeof(my_semaphore));
    if (s) {
        *s = CreateSemaphore(NULL, initialValue, maxValue, NULL);
    }
    return s;
}
void my_destroySemaphore(my_semaphore* s) {
    if (s) {
        CloseHandle(*s);
        free(s);
    }
}
void my_waitSemaphore(my_semaphore* s) {
    WaitForSingleObject(*s, INFINITE);
}
void my_postSemaphore(my_semaphore* s) {
    ReleaseSemaphore(*s, 1, NULL);
}

my_thread* my_createThread(void* (*start_routine)(void*), void* arg) {
    my_thread* t = (my_thread*)malloc(sizeof(my_thread));
    if (t) {
        // LPTHREAD_START_ROUTINE 是 Windows 下线程函数的类型定义，等价于 DWORD (WINAPI *)(LPVOID)
        *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
    }
    return t;
}

unsigned long my_get_thread_id() {
    return (unsigned long)GetCurrentThreadId();
}

#else

my_mutex* my_createMutex()
{
    my_mutex* m = (my_mutex*)malloc(sizeof(my_mutex));
    if (m) {
        pthread_mutex_init(m, NULL);//指针，NULL表示默认属性
    }
    return m;
}
void my_destroyMutex(my_mutex* m)
{
    if (m) {
        pthread_mutex_destroy(m);
        free(m);
    }
}
void my_lockMutex(my_mutex* m) {
    pthread_mutex_lock(m);
}
void my_unlockMutex(my_mutex* m) {
    pthread_mutex_unlock(m);
}

my_semaphore* my_createSemaphore(unsigned int initialValue, unsigned int maxValue) {
    // maxValue参数在POSIX信号量中无效，仅用initialValue
    my_semaphore* s = (my_semaphore*)malloc(sizeof(my_semaphore));
    if (s) {
        sem_init(s, 0, initialValue);
    }
    return s;
}
void my_destroySemaphore(my_semaphore* s) {
    if (s) {
        sem_destroy(s);
        free(s);
    }
}
void my_waitSemaphore(my_semaphore* s) {
    sem_wait(s);
}
void my_postSemaphore(my_semaphore* s) {
    sem_post(s);
}

my_thread* my_createThread(void* (*start_routine)(void*), void* arg) {
    my_thread* t = (my_thread*)malloc(sizeof(my_thread));
    if (t) {
        pthread_create(t, NULL, start_routine, arg);
    }
    return t;
}

unsigned long my_get_thread_id() {
    return (unsigned long)pthread_self();
}

#endif