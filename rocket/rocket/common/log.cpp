#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/common/mutex.h"
#include "rocket/common/run_time.h"
#include "rocket/common/util.h"
#include "rocket/net/eventLoop.h"
#include <csignal>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <unistd.h>

namespace rocket {

static Logger* g_logger = nullptr;

void CoredumpHandler(int signal_no) {
	ERRORLOG("progress receoved invalid signal, will exit");
	g_logger->flush();
	pthread_join(g_logger->getAsyncLogger()->m_thread, nullptr);
	pthread_join(g_logger->getAsyncAppLogger()->m_thread, nullptr);
	signal(signal_no, SIG_DFL);
	raise(signal_no);
}

Logger* Logger::getGlobalLogger() { return g_logger; }

Logger::Logger(LogLevel level, int type)
    : m_set_level(level)
    , m_type(type) {

	if (m_type == 0) {
		return;
	}
	m_async_logger = std::make_shared<AsyncLogger>(
	    Config::GetGlobalConfig()->m_log_file_name + "_rpc",
	    Config::GetGlobalConfig()->m_log_file_path,
	    Config::GetGlobalConfig()->m_log_max_file_size);
	m_async_app_logger = std::make_shared<AsyncLogger>(
	    Config::GetGlobalConfig()->m_log_file_name + "_app",
	    Config::GetGlobalConfig()->m_log_file_path,
	    Config::GetGlobalConfig()->m_log_max_file_size);
}

Logger::~Logger() { m_timer_event->setCanceled(true); }

void Logger::InitGlobalLogger(int type /* = 1*/) {
	LogLevel global_log_level =
	    StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
	std::cout << "Init global log level is "
	          << Config::GetGlobalConfig()->m_log_level << std::endl;
	g_logger = new Logger(global_log_level, type);
	g_logger->init();
}

void Logger::init() {

	if (m_type == 0) {
		return;
	}
	m_timer_event = std::make_shared<TimerEvent>(
	    Config::GetGlobalConfig()->m_log_flush_interval, true,
	    std::bind(&Logger::syncLoop, this));

	EventLoop::getCurrentEventLoop()->addTimerEvent(m_timer_event);

	// 注册信号处理函数
	signal(SIGSEGV, CoredumpHandler);   // 处理段错误
	signal(SIGABRT, CoredumpHandler);   // 处理abort
	signal(SIGTERM, CoredumpHandler);   // 处理kill -15
	signal(SIGKILL, CoredumpHandler);   // 处理kill -9
	signal(SIGINT, CoredumpHandler);    // 处理Ctrl+C
	signal(SIGSTKFLT, CoredumpHandler); // 处理栈溢出
}

void Logger::pushLog(const std::string& msg) {
	if (m_type == 0) {
		std::cout << msg << '\n';
		return;
	}
	ScopeMutex<Mutex> lock(m_mutex);
	m_buffers.push_back(msg);
	lock.unlock();
}

void Logger::pushAppLog(const std::string& msg) {
	ScopeMutex<Mutex> lock(m_app_mutex);
	m_app_buffers.push_back(msg);
	lock.unlock();
}

void Logger::log() {}

void Logger::syncLoop() {
	// 同步m_buffer 到 async_logger 的buffer队尾
	printf("sync to async logger\n");
	std::vector<std::string> tmp_vec;
	ScopeMutex<Mutex> lock(m_mutex);
	m_buffers.swap(tmp_vec);
	lock.unlock();
	if (!tmp_vec.empty()) {
		m_async_logger->pushLogBuffer(tmp_vec);
	}
	tmp_vec.clear();

	// =========================== //

	std::vector<std::string> tmp_vec_app;
	ScopeMutex<Mutex> lock_app(m_app_mutex);
	m_app_buffers.swap(tmp_vec_app);
	lock_app.unlock();

	if (!tmp_vec_app.empty()) {
		m_async_app_logger->pushLogBuffer(tmp_vec_app);
	}
	tmp_vec_app.clear();
}

void Logger::flush() {
	syncLoop();
	m_async_logger->stop();
	m_async_logger->flush();

	m_async_app_logger->stop();
	m_async_app_logger->flush();
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

	// 获取当前线程处理的请求的msgid
	auto msgId = RunTime::getRuntime().getMsgId();
	auto methodName = RunTime::getRuntime().getMethodName();

	if (!msgId.empty()) {
		ss << "[" << msgId << "]\t";
	}

	if (!methodName.empty()) {
		ss << "[" << methodName << "]\t";
	}
	return ss.str();
}

AsyncLogger::AsyncLogger(const std::string& file_name,
                         const std::string& file_path, int max_size)
    : m_file_name(file_name)
    , m_file_path(file_path)
    , m_max_file_size(max_size) {
	sem_init(&m_semaphore, 0, 0);

	pthread_cond_init(&m_condition, nullptr);
	pthread_create(&m_thread, nullptr, &AsyncLogger::Loop, this);

	sem_wait(&m_semaphore);
}

AsyncLogger::~AsyncLogger() {
	pthread_join(m_thread, nullptr);
	pthread_cond_destroy(&m_condition);
	sem_destroy(&m_semaphore);
}

void* AsyncLogger::Loop(void* arg) {
	// 将buffer里面的全部数据打印到文件中，然后线程睡眠，知道有新的数据再重复这个过程
	AsyncLogger* logger = reinterpret_cast<AsyncLogger*>(arg);

	sem_post(&logger->m_semaphore);

	while (true) {
		ScopeMutex<Mutex> lock(logger->m_mutex);
		while (logger->m_buffers.empty()) {
			pthread_cond_wait(&logger->m_condition, logger->m_mutex.getMutex());
		}
		printf("pthread_cond_wait back\n");

		std::vector<std::string> tmp = logger->m_buffers.front();
		logger->m_buffers.pop();
		lock.unlock();

		timeval now;
		gettimeofday(&now, nullptr);

		struct tm now_time;
		localtime_r(&(now.tv_sec), &now_time);

		const char* format = "%Y%m%d";

		char date[32];
		strftime(date, sizeof(date), format, &now_time);

		if (std::string(date) != logger->m_date) {
			logger->m_reopen_flag = true;
			logger->m_no = 0;
			logger->m_date = std::string(date);
		}
		if (logger->m_file_handler == nullptr) {
			logger->m_reopen_flag = true;
		}

		std::stringstream ss;
		ss << logger->m_file_path << "/" << logger->m_file_name << "_"
		   << std::string(date) << "_log.";
		std::string log_file_name = ss.str() + std::to_string(logger->m_no);

		if (logger->m_reopen_flag) {
			if (logger->m_file_handler != nullptr) {
				fclose(logger->m_file_handler);
			}
			logger->m_file_handler = fopen(log_file_name.c_str(), "a");
			logger->m_reopen_flag = false;
		}

		if (ftell(logger->m_file_handler) > logger->m_max_file_size) {
			fclose(logger->m_file_handler);
			log_file_name = ss.str() + std::to_string(++logger->m_no);
			logger->m_file_handler = fopen(log_file_name.c_str(), "a");
			logger->m_reopen_flag = false;
		}

		for (const auto& msg : tmp) {
			if (!msg.empty()) {
				fwrite(msg.c_str(), 1, msg.size(), logger->m_file_handler);
			}
		}

		fflush(logger->m_file_handler); // 刷新缓冲区

		if (logger->m_stop_flag) {
			return nullptr;
		}
	}
	return nullptr;
}

void AsyncLogger::stop() {
	m_stop_flag = true;
	// pthread_cond_signal(&m_condition);
	// pthread_join(m_thread, nullptr);
}

void AsyncLogger::flush() {
	if (m_file_handler != nullptr) {
		fflush(m_file_handler);
	}
}

void AsyncLogger::pushLogBuffer(const std::vector<std::string>& msg) {
	ScopeMutex<Mutex> lock(m_mutex);
	m_buffers.push(msg);
	pthread_cond_signal(&m_condition);
	lock.unlock();

	// 唤醒async_logger的线程
	printf("pthread_cond_signal\n");
}

} // namespace rocket