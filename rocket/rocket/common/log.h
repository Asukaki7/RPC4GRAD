#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <cstdint>
#include <cstdio>
#include <memory>
#include <queue>
#include <string>

namespace rocket {

template <typename... Args>
std::string formatString(const char* str, Args&&... args) {
	int size = snprintf(nullptr, 0, str, args...);

	std::string result;
	if (size > 0) {
		result.resize(size);
		snprintf(&result[0], size + 1, str, args...);
	}

	return result;
}

#define DEBUGLOG(str, ...)                                            \
	std::string msg =                                                 \
	    (new rocket::LogEvent(rocket::LogLevel::Debug))->toString() + \
	    rocket::formatString(str, ##__VA_ARGS__);                     \
	msg += '\n';                                                      \
	rocket::Logger::getGlobalLogger()->pushLog(msg);                  \
	rocket::Logger::getGlobalLogger()->log();

enum LogLevel { Debug = 1, Info = 2, Error = 3 };

std::string LogLevelToString(LogLevel level);

class Logger {
public:
	typedef std::shared_ptr<Logger> s_ptr;
	void pushLog(const std::string& msg);
	void log();

public:
	static Logger* getGlobalLogger();

private:
	LogLevel m_set_level;
	std::queue<std::string> m_buffers;
};

class LogEvent {
public:
	// 构造函数
	LogEvent(LogLevel level)
	    : m_level(level) {}

	// 获取文件名
	const std::string& getFileName() const { return m_file_name; }

	// 获取日志级别
	LogLevel getLogLevel() const { return m_level; }

	std::string toString();

private:
	std::string m_file_name; // filename
	int32_t m_file_line;     // line number
	int32_t m_pid;           // process id
	int32_t m_thred_id;      // thread id

	LogLevel m_level;

	Logger::s_ptr m_logger;
};

} // namespace rocket

#endif