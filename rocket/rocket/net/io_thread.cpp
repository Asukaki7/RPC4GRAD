#include "rocket/net/io_thread.h"
#include "rocket/net/eventLoop.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include <cassert>
#include <cstddef>
#include <pthread.h>
#include <semaphore.h>

namespace rocket {
    
IOThread::IOThread() {
    int rt = sem_init(&m_init_semaphore, 0, 0);
    assert(rt == 0);

    rt = sem_init(&m_start_semaphore, 0, 0);
    assert(rt == 0);

    pthread_create(&m_thread, NULL, &IOThread::Main, this);

    // 通过一个信号量 wait 直到新线程执行完Main函数的前置操作(即准备动作 不是真正的loop循环)
    sem_wait(&m_init_semaphore);

    DEBUGLOG("IOThread [%d] create success", m_thread_id);
}

IOThread::~IOThread(){
    m_event_loop->stop();
    sem_destroy(&m_init_semaphore);
    sem_destroy(&m_start_semaphore);

    pthread_join(m_thread, nullptr);

    if (m_event_loop != nullptr) {
        delete m_event_loop;
        m_event_loop = nullptr;
    }
}

void* IOThread::Main(void* arg) {
    auto thread = static_cast<IOThread*>(arg);

    thread->m_event_loop = new EventLoop();
    thread->m_thread_id = getThreadId(); // 此时已经处于一个新的线程了

    // 唤醒等待的信号量
    sem_post(&thread->m_init_semaphore);

    // 让io线程等待 直到我们主动的启动
    DEBUGLOG("IOthread [%d] created, wait start semaphore", thread->m_thread_id);
    sem_wait(&thread->m_start_semaphore);
    DEBUGLOG("IOthread [%d] start loop", thread->m_thread_id);
    thread->m_event_loop->loop();
    DEBUGLOG("IOthread [%d] end loop", thread->m_thread_id);

    return nullptr;
}

void IOThread::start(){
    DEBUGLOG("now invoke IOthread [%d]", m_thread_id);
    sem_post(&m_start_semaphore);
}

void IOThread::join() {
    pthread_join(m_thread, nullptr);
}





}