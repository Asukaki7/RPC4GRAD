#ifndef ROCKET_COMMON_ERROR_EXCEPTION_H
#define ROCKET_COMMON_ERROR_EXCEPTION_H

#include <exception>

namespace rocket {
class RocketException : public std::exception {

public:
	RocketException() = default;

    // 异常处理
    // 当eventloop捕获到RocketException及其子类对象异常，会执行该函数
    virtual void handle() = 0;

    virtual ~RocketException() {};
};
} // namespace rocket

#endif