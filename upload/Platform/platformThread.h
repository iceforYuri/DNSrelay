#include "header.h"

#ifdef _WIN32

#include <windows.h>
typedef HANDLE my_thread;
typedef HANDLE my_mutex;
typedef HANDLE my_semaphore;

my_mutex* my_createMutex();
void my_destroyMutex(my_mutex* m);
void my_lockMutex(my_mutex* m);
void my_unlockMutex(my_mutex* m);

my_semaphore* my_createSemaphore(unsigned int initialValue,unsigned int maxValue);
void my_destroySemaphore(my_semaphore* m);
void my_waitSemaphore(my_semaphore* s);
void my_postSemaphore(my_semaphore* s);

my_thread* my_createThread(void* (*start_routine)(void*), void* arg);
unsigned long my_get_thread_id();

#else
#include <pthread.h>
#include <semaphore.h>

typedef pthread_t my_thread;
typedef pthread_mutex_t my_mutex;
typedef sem_t my_semaphore;

my_mutex* my_createMutex();
void my_destroyMutex(my_mutex* m);
void my_lockMutex(my_mutex* m);
void my_unlockMutex(my_mutex* m);

my_semaphore* my_createSemaphore(unsigned int initialValue,unsigned int maxValue);
void my_destroySemaphore(my_semaphore* m);
void my_waitSemaphore(my_semaphore* s);
void my_postSemaphore(my_semaphore* s);

my_thread* my_createThread(void* (*start_routine)(void*), void* arg);
unsigned long my_get_thread_id();

#endif