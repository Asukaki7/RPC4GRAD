#ifndef ROCKET_NET_IO_THREAD_H
#define ROCKET_NET_IO_THREAD_H


#include "rocket/net/eventLoop.h"
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

namespace rocket {
class IOThread {

public:
IOThread();
~IOThread();
EventLoop* geteventloop() {return m_event_loop;}
void start();
void join();

public:
    static void* Main(void* arg);
private:
    pid_t m_thread_id {-1}; // 线程id
    pthread_t m_thread {}; // 线程句柄
    EventLoop* m_event_loop {nullptr}; // 当前io线程的事件循环
    sem_t m_init_semaphore {};
    sem_t m_start_semaphore {};
};
}


#endif