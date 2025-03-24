#include "log.h"
#include "util.h"
#include <ctime>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <iostream>



namespace rocket {

static Logger* g_logger = nullptr;

Logger* Logger::getGlobalLogger() {
    if (g_logger == nullptr) {
        g_logger = new Logger();
    }
    return g_logger;
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
	   << "[" << m_pid << ":" << m_thred_id << "]\t"
	   << "[" << std::string(__FILE__) << ":" << __LINE__ << "]\t";

	return ss.str();
}

void Logger::pushLog(const std::string& msg) {
    m_buffers.push(msg);
}

void Logger::log() {
    while (!m_buffers.empty()) {
        std::cout << m_buffers.front() << std::endl;
        m_buffers.pop();
    }
}
} // namespace rocket