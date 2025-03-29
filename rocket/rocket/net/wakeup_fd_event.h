#ifndef ROCKT_NET_WAKEUP_FD_EVENT_H
#define ROCKT_NET_WAKEUP_FD_EVENT_H
#include "rocket/net/fd_event.h"

namespace rocket {

class WakeupFdEvent : public FdEvent {
public:
	WakeupFdEvent(int fd);
	~WakeupFdEvent();

    void wakeup();
};

} // namespace rocket

#endif