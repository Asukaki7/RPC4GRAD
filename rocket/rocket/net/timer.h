#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/timer_event.h"
#include <cstdint>
#include <map>

namespace rocket {

class Timer :public FdEvent {

public:
	Timer();

	~Timer();

	void addTimerEvent(TimerEvent::s_ptr timer_event);

	void deleteTimerEvent(TimerEvent::s_ptr timer_event);

	void onTimer(); // 当发生IO事件后 eventloop会执行这个回调函数 bind 到对应fd的cb上
private:
    void resetArriveTime();
private:
	Mutex m_mutex;
	std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events; 
};
} // namespace rocket
#endif