#include "rocket/net/eventLoop.h"
#include "fd_event.h"
#include "rocket/common/log.h"
#include "rocket/common/mutex.h"
#include "rocket/common/util.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <queue>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#define ADD_TO_EPOLL()                                                   \
	auto it = m_listen_fds.find(event->getFd());                         \
	int op = EPOLL_CTL_ADD;                                              \
	if (it != m_listen_fds.end()) {                                      \
		op = EPOLL_CTL_MOD;                                              \
	}                                                                    \
	epoll_event tmp = event->getEpollEvent();     \
	INFOLOG("epoll_event.events = %d", (int)tmp.events);                       \
	int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);            \
	if (rt == -1) {                                                      \
		ERRORLOG("faild epoll_ctrl when add fd,errno = %d, error = %s ", \
		         errno, strerror(errno));                                \
	}                                                                    \
	DEBUGLOG("add event success, fd[%d]", event->getFd());

#define DELETE_FROM_EPOLL()                                                 \
	auto it = m_listen_fds.find(event->getFd());                            \
	if (it == m_listen_fds.end()) {                                         \
		return;                                                             \
	}                                                                       \
	int op = EPOLL_CTL_DEL;                                                 \
	epoll_event tmp = event->getEpollEvent();                               \
	INFOLOG("epoll_event.events = %d", (int)tmp.events);                       \
	int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);               \
	if (rt == -1) {                                                         \
		ERRORLOG("faild epoll_ctrl when delete fd,errno = %d, error = %s ", \
		         errno, strerror(errno));                                   \
	}                                                                       \
	DEBUGLOG("delete event success [%d]", event->getFd());                  \
	m_listen_fds.erase(it);

namespace rocket {

static thread_local EventLoop* t_current_eventloop = nullptr;
static int g_epoll_max_timeout = 10000;
static const int g_epoll_max_event = 10;

EventLoop::EventLoop() {
	if (t_current_eventloop != nullptr) {
		ERRORLOG(
		    "faild to create eventloop, this thread has create event loop");
		std::exit(1);
	}
	m_thread_id = getThreadId();

	m_epoll_fd = epoll_create(10);

	if (m_epoll_fd < 0) {
		ERRORLOG(
		    "faild to create epoll loop, epoll_create error ,error info[%d]",
		    errno);
		std::exit(1);
	}

	initWakeupFdEvent();

	// epoll_event event;
	// int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_wakeup_fd, &event);
	// if (rt == -1) {
	// 	ERRORLOG("faild to create epoll loop, epoll_ctl error ,error info[%d]",
	// 	         errno);
	// 	std::exit(1);
	// }
	DEBUGLOG("succ create eventloop in thread %d", m_thread_id);
	t_current_eventloop = this;
}
EventLoop::~EventLoop() {
	close(m_epoll_fd);
	if (m_wakeup_fd_event != nullptr) {
		delete m_wakeup_fd_event;
		m_wakeup_fd_event = nullptr;
	} 
}

void EventLoop::loop() {
	while (!m_stop_flag) {
		ScopeMutex<Mutex> lock(m_mutex);
		std::queue<std::function<void()>> tmp_tasks{};
		m_pending_tasks.swap(tmp_tasks);
		lock.unlock();

		while (!tmp_tasks.empty()) {
			auto cb = tmp_tasks.front();
			tmp_tasks.pop();
			if (cb) {
				cb();
			}
		}

		int timeout = g_epoll_max_timeout;
		epoll_event result_events[g_epoll_max_event];
		// DEBUGLOG("now begin to epoll_wait");
		int rt =
		    epoll_wait(m_epoll_fd, result_events, g_epoll_max_event, timeout);

		DEBUGLOG("now end to epoll_wait rt = %d", rt);

		if (rt == -1) {
			ERRORLOG("epoll_wait error, errno = ", errno);
		} else {
			for (int i = 0; i < rt; i++) {
				epoll_event trigger_event = result_events[i];
				auto fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
				if (fd_event == nullptr) {
					continue;
				}
				if (trigger_event.events & EPOLLIN) {
					DEBUGLOG("fd %d trigger EPOLLIN event", fd_event->getFd());
					addTask(fd_event->handler(FdEvent::TriggerEvent::IN_EVENT));
				}
				if (trigger_event.events & EPOLLOUT) {
					DEBUGLOG("fd %d trigger EPOLLOUT event", fd_event->getFd());
					addTask(fd_event->handler(FdEvent::TriggerEvent::OUT_EVENT));
				}
			}
		}
	}
}

void EventLoop::wakeup() {
	m_wakeup_fd_event->wakeup();
}

void EventLoop::stop() {
	m_stop_flag = true;
}

void EventLoop::dealWakeup() {}

void EventLoop::addEpollEvent(FdEvent* event) {
	if (isInLoopThread()) {
		ADD_TO_EPOLL();
	} else {
		auto cb = [this, event]() { ADD_TO_EPOLL(); };
		addTask(cb,true);
	}
}

void EventLoop::deleteEpollEvent(FdEvent* event) {
	if (isInLoopThread()) {
		DELETE_FROM_EPOLL() 
	} else {
		auto cb = [this, event]() {
			DELETE_FROM_EPOLL() 
		};
		addTask(cb, true);
	}
}

void EventLoop::addTask(std::function<void()> cb, bool is_wake_up /* = false */) {
	ScopeMutex<Mutex> lock(m_mutex);
	m_pending_tasks.push(cb);
	lock.unlock();

	if (is_wake_up) {
		wakeup();
	}
}

void EventLoop::initWakeupFdEvent() {
	m_wakeup_fd = eventfd(0, EFD_NONBLOCK);

	if (m_wakeup_fd < 0) {
		ERRORLOG("faild to create epoll loop, eventfd error ,error info[%d]",
		         errno);
		std::exit(1);
	}

	INFOLOG("wakeup fd is %d", m_wakeup_fd);
	m_wakeup_fd_event = new WakeupFdEvent(m_wakeup_fd);

	m_wakeup_fd_event->listenRead(FdEvent::IN_EVENT, [this]() {
        char buf[8];
        while (read(m_wakeup_fd, buf, 8) != -1 && errno != EAGAIN) {
        }
        DEBUGLOG("read full bytes from wakeup fd, fd[%d]", m_wakeup_fd);
    });

	addEpollEvent(m_wakeup_fd_event);
}



bool EventLoop::isInLoopThread() const { return getThreadId() == m_thread_id; }
} // namespace rocket