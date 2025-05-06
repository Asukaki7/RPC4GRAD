#ifndef ROCKET_COMMON_EXCEPTION_H
#define ROCKET_COMMON_EXCEPTION_H

#include <exception>
#include <string>
namespace rocket {
class RocketException : public std::exception {

public:
	RocketException(int err_code, const std::string& err_info)
	    : m_err_code(err_code)
	    , m_err_info(err_info) {}

	// 异常处理
	// 当捕获到RocketException及其子类对象异常，会执行该函数
	virtual void handle() = 0;

	virtual ~RocketException(){};

	int errorCode() const { return m_err_code; }
	std::string errorInfo() const { return m_err_info; }

protected:
	int m_err_code;
	std::string m_err_info;
};
} // namespace rocket

#endif