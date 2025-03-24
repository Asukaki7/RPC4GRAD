#include "util.h"


static int g_pid = 0;

static thread_local int g_thread_id = 0;

namespace rocket {
    pid_t getPid() {
        if (g_pid == 0) {
            g_pid = getpid();
        }
        return g_pid;
    }

    pid_t getThreadId() {
        if (g_thread_id == 0) {
            g_thread_id = syscall(SYS_gettid);
        }
        return g_thread_id;
    }

}

/* 线程安全版本
#include "util.h"
#include <atomic>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <cstdio>
#include <mutex>
#include <thread>
namespace rocket {
    static std::atomic<pid_t> g_pid{0};
    static std::atomic<pid_t> g_thread_id{0};
    static std::once_flag pid_flag;      // 确保 getpid() 只调用一次
    static std::once_flag thread_flag;    // 确保 syscall(SYS_gettid) 只调用一次

    pid_t getPid() {
        std::call_once(pid_flag, []() {
            g_pid.store(getpid(), std::memory_order_relaxed);
        });
        return g_pid.load(std::memory_order_relaxed);
    }

    pid_t getThreadId() {
        std::call_once(thread_flag, []() {
            g_thread_id.store(static_cast<pid_t>(syscall(SYS_gettid)), std::memory_order_relaxed);
        });
        return g_thread_id.load(std::memory_order_relaxed);
    }
}
*/