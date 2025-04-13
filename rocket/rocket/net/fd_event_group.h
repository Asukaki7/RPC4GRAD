#ifndef ROCKET_NET_FD_EVENT_GROUP_H
#define ROCKET_NET_FD_EVENT_GROUP_H
#include "rocket/net/fd_event.h"
#include <vector>
#include "rocket/common/mutex.h"
namespace rocket {
class FdEventGroup {
public:
    FdEventGroup(int size);
    ~FdEventGroup();
    FdEvent* getFdEvent(int index);

public:
    static FdEventGroup* getFdEventGroup();

private:
    int m_size {};
    std::vector<FdEvent*> m_fd_groups {};
    Mutex m_mutex {};
};
}


#endif