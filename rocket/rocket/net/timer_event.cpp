#include "rocket/net/timer_event.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include <sys/time.h>

namespace rocket {
TimerEvent::TimerEvent(int interval, bool is_repeated, std::function<void()> cb)
    : m_interval(interval)
    , m_is_repeated(is_repeated)
    , m_task(cb) {
	resetArriveTime();
}

void TimerEvent::resetArriveTime() { 
    auto a = getNowMs();
	m_arrive_time = a + m_interval;
    // DEBUGLOG("success create timer event, will execute at [%lld]",
    //     m_arrive_time);
}

} // namespace rocket