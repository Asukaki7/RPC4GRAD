#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include "rocket/common/config.h"
#include "rocket/common/mutex.h"
#include "rocket/net/timer_event.h"
#include <cstdint>
#include <cstdio>
#include <memory>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <string>
#include <vector>

namespace rocket {

template <typename... Args>
std::string formatString(const char* fmt, Args&&... args) {
	int size = snprintf(nullptr, 0, fmt, args...);
	std::string result{};
	if (size > 0) {
		result.resize(size);
		snprintf(&result[0], size + 1, fmt, args...);
	}
	return result;
}

#define DEBUGLOG(str, ...)                                                     \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() &&   		    	\
		 	rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Debug) {                                         \
			auto msg =                                                         \
			    (rocket::LogEvent(rocket::LogLevel::Debug)).toString() + "[" + \
			    std::string(__FILE__) + ":" + std::to_string(__LINE__) +       \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';       \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
			}                                                                  \
		}                                                                      \
	} while (0)

#define INFOLOG(str, ...)                                                      \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Info) {                                          \
			auto msg = (rocket::LogEvent(rocket::LogLevel::Info)).toString() + \
			           "[" + std::string(__FILE__) + ":" +                     \
			           std::to_string(__LINE__) + "]\t" +                      \
			           rocket::formatString(str, ##__VA_ARGS__) + '\n';        \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
			}                                                                  \
		}                                                                      \
	} while (0)

#define ERRORLOG(str, ...)                                                     \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Error) {                                         \
			auto msg =                                                         \
			    (rocket::LogEvent(rocket::LogLevel::Error)).toString() + "[" + \
			    std::string(__FILE__) + ":" + std::to_string(__LINE__) +       \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';       \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushLog(msg);               \
			}                                                                  \
		}                                                                      \
	} while (0)


#define APPDEBUGLOG(str, ...)                                                     \
do {                                                                       \
	if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		rocket::LogLevel::Debug) {                                         \
		auto msg =                                                         \
			(rocket::LogEvent(rocket::LogLevel::Debug)).toString() + "[" + \
			std::string(__FILE__) + ":" + std::to_string(__LINE__) +       \
			"]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';       \
		if (msg.size() < msg.max_size()) {                                 \
			rocket::Logger::getGlobalLogger()->pushAppLog(msg);               \
		}                                                                  \
	}                                                                      \
} while (0)

#define APPINFOLOG(str, ...)                                                      \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Info) {                                          \
			auto msg = (rocket::LogEvent(rocket::LogLevel::Info)).toString() + \
			           "[" + std::string(__FILE__) + ":" +                     \
			           std::to_string(__LINE__) + "]\t" +                      \
			           rocket::formatString(str, ##__VA_ARGS__) + '\n';        \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushAppLog(msg);               \
			}                                                                  \
		}                                                                      \
	} while (0)

#define APPERRORLOG(str, ...)                                                     \
	do {                                                                       \
		if (rocket::Logger::getGlobalLogger()->getlogLevel() <=                \
		    rocket::LogLevel::Error) {                                         \
			auto msg =                                                         \
			    (rocket::LogEvent(rocket::LogLevel::Error)).toString() + "[" + \
			    std::string(__FILE__) + ":" + std::to_string(__LINE__) +       \
			    "]\t" + rocket::formatString(str, ##__VA_ARGS__) + '\n';       \
			if (msg.size() < msg.max_size()) {                                 \
				rocket::Logger::getGlobalLogger()->pushAppLog(msg);               \
			}                                                                  \
		}                                                                      \
	} while (0)



enum LogLevel { Unknown = 0, Debug = 1, Info = 2, Error = 3 };

std::string LogLevelToString(LogLevel level);
LogLevel StringToLogLevel(const std::string& log_level);




class AsyncLogger {

public:
	typedef std::shared_ptr<AsyncLogger> s_ptr;
	AsyncLogger(const std::string& file_name, const std::string& file_path,
	            int max_size);
	~AsyncLogger();
	void stop();
	void flush();
	void pushLogBuffer(const std::vector<std::string>& msg);

public:
	static void* Loop(void* arg);

private:
	std::queue<std::vector<std::string>> m_buffers;
	std::queue<std::vector<std::string>> m_app_buffers;

	// m_file_path/m_file_name_yymmdd.no
	std::string m_file_name; // 文件名
	std::string m_file_path; // 文件路径
	int m_no{0};             // 日志序号

	int m_max_file_size{}; // 日志单个文件大小

	sem_t m_semaphore;
	pthread_t m_thread;

	pthread_cond_t m_condition;
	Mutex m_mutex;

	std::string m_date;                 // 当前打印日志的文件日期
	std::FILE* m_file_handler{nullptr}; // 当前打印日志的文件句柄

	bool m_reopen_flag{false}; // 是否需要重新打开文件
	bool m_stop_flag{false};   // 是否停止loop
};

class Logger {
public:
	typedef std::shared_ptr<Logger> s_ptr;

	Logger(LogLevel level, int type = 1);
	~Logger();
	Logger() = default;

	void pushLog(const std::string& msg);
	void pushAppLog(const std::string& msg);

	void log();

	LogLevel getlogLevel() const { return m_set_level; }

	void syncLoop();
	void init();

public:
	static Logger* getGlobalLogger();
	static void InitGlobalLogger(int type = 1);

private:
	LogLevel m_set_level;
	std::vector<std::string> m_buffers;
	std::vector<std::string> m_app_buffers;

	Mutex m_mutex;
	Mutex m_app_mutex;

	std::string m_file_name; // 文件名
	std::string m_file_path; // 文件路径

	int m_max_file_size{0}; // 日志单个文件大小
	int m_type{0};          // 日志类型 0 表示sync日志，1 表示async日志

	// rpc系统Logger
	AsyncLogger::s_ptr m_async_logger;
	// app系统运行Logger
	AsyncLogger::s_ptr m_async_app_logger;

	TimerEvent::s_ptr m_timer_event;
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