#include "log.h"
#include "mutex.h"
#include "util.h"
#include <ctime>
#include <queue>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <iostream>
#include "common/config.h"


namespace rocket {

static Logger* g_logger = nullptr;

Logger* Logger::getGlobalLogger() {
    return g_logger;
}


void Logger::InitGlobalLogger() {
	LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    std::cout << "Init global log level is " << Config::GetGlobalConfig()->m_log_level << std::endl;
	g_logger = new Logger(global_log_level);
}

std::string LogLevelToString(LogLevel level) {
	switch (level) {
	case LogLevel::Debug:
		return "DEBUG";
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Error:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

LogLevel StringToLogLevel(const std::string& log_level) {
	if (log_level == "DEBUG") {
		return LogLevel::Debug;
	} else if (log_level == "INFO") {
		return LogLevel::Info;
	} else if (log_level == "ERROR") {
		return LogLevel::Error;
	} else {
		return LogLevel::Unknown;
	}
}

std::string LogEvent::toString() {
	struct timeval now_time;

	gettimeofday(&now_time, nullptr);

	struct tm now_time_t;

	localtime_r(&now_time.tv_sec, &now_time_t);

	char buf[128];
	strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);
	std::string time_str(buf);

	int ms = now_time.tv_usec / 1000;
	time_str = time_str + "." + std::to_string(ms);

	m_pid = getPid();
	m_thred_id = getThreadId();

	std::stringstream ss;

	ss << "[" << LogLevelToString(m_level) << "]\t"
	   << "[" << time_str << "]\t"
	   << "[" << m_pid << ":" << m_thred_id << "]\t";

	return ss.str();
}

void Logger::pushLog(const std::string& msg) {
	ScopeMutex<Mutex> lock(m_mutex);
    m_buffers.push(msg);
	lock.unlock();
}

void Logger::log() {
	ScopeMutex<Mutex> lock(m_mutex);
	std::queue<std::string> tmp;
	m_buffers.swap(tmp);
	lock.unlock();
    while (!tmp.empty()) {
        std::cout << tmp.front() << std::endl;
        tmp.pop();
    }
	
}
} // namespace rocket