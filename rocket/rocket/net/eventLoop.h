#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/net/timer.h"
#include "rocket/net/timer_event.h"
#include <functional>
#include <pthread.h>
#include <queue>
#include <set>
namespace rocket {
// only one eventloop per thread
class EventLoop {

public:
	EventLoop();
	~EventLoop();
	void loop();

	void wakeup();

	void stop();

	void addEpollEvent(FdEvent* event);
	void deleteEpollEvent(FdEvent* event);

	bool isInLoopThread() const;
	void addTask(std::function<void()> cb, bool is_wake_up = false); // add task to task queue
	void addTimerEvent(TimerEvent::s_ptr event);

private:
	void dealWakeup();
	void initWakeupFdEvent();
	void initTimer();

private:
	pid_t m_thread_id{0};
	int m_epoll_fd{0};
	int m_wakeup_fd{0};
	bool m_stop_flag{false};
	WakeupFdEvent* m_wakeup_fd_event {nullptr}; // wakeup fd
	std::set<int> m_listen_fds; // fd set
	std::queue<std::function<void()>> m_pending_tasks; // task queue
	Mutex m_mutex;
	Timer* m_timer{nullptr};
};
} // namespace rocket

#endif