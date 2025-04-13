#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include "rocket/common/mutex.h"
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

#define DEBUGLOG(str, ...)                                                     \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Debug) {                                         \
			auto msg =                                                         \
			    (new rocket::LogEvent(rocket::LogLevel::Debug))->toString() +  \
			    "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';      \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
				rocket::Logger::getGlobalLogger()->log();                      \
			}                                                                  \
		}                                                                      \
	} while (0)

#define INFOLOG(str, ...)                                                      \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Info) {                                          \
			auto msg =                                                         \
			    (new rocket::LogEvent(rocket::LogLevel::Info))->toString() +   \
			    "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';      \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
				rocket::Logger::getGlobalLogger()->log();                      \
			}                                                                  \
		}                                                                      \
	} while (0)

#define ERRORLOG(str, ...)                                                     \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Error) {                                         \
			auto msg =                                                         \
			    (new rocket::LogEvent(rocket::LogLevel::Error))->toString() +  \
			    "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';      \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
				rocket::Logger::getGlobalLogger()->log();                      \
			}                                                                  \
		}                                                                      \
	} while (0)

enum LogLevel { Unknown = 0, Debug = 1, Info = 2, Error = 3 };

std::string LogLevelToString(LogLevel level);
LogLevel StringToLogLevel(const std::string& log_level);

class Logger {
public:
	typedef std::shared_ptr<Logger> s_ptr;

	Logger(LogLevel level)
	    : m_set_level(level){};
	Logger() = default;
	void pushLog(const std::string& msg);
	void log();

	LogLevel getlogLevel() const { return m_set_level; }

public:
	static Logger* getGlobalLogger();
	static void InitGlobalLogger();

private:
	LogLevel m_set_level;
	std::queue<std::string> m_buffers;

	Mutex m_mutex;
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