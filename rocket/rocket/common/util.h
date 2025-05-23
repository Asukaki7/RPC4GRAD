#ifndef ROCKET_COMMON_UTIL_H
#define ROCKET_COMMON_UTIL_H


#include <cstdint>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

namespace rocket {
    pid_t getPid();
    pid_t getThreadId();
    int64_t getNowMs();
    int32_t getInt32FromNetByte(const char* buf);
}

#endif // ROCKET_COMMON_UTIL_H