
#include "rocket/net/fd_event_group.h"
#include "rocket/common/mutex.h"
#include <cstddef>

namespace rocket {

static FdEventGroup* g_fd_event_group = nullptr;


FdEventGroup* FdEventGroup::getFdEventGroup() {
    if (g_fd_event_group != nullptr) {
        return g_fd_event_group;
    }

    g_fd_event_group = new FdEventGroup(128);
    
    return g_fd_event_group;
}


FdEventGroup::FdEventGroup(int size) : m_size(size){
    for (int i = 0; i < m_size; i++) {
        m_fd_groups.push_back(new FdEvent(i));
    }
}

FdEventGroup::~FdEventGroup() {
    for (int i = 0; i < m_size; i++) {
        if (m_fd_groups[i] != nullptr) {
            delete m_fd_groups[i];
            m_fd_groups[i] = nullptr;
        }
    }
}

FdEvent* FdEventGroup::getFdEvent(int index) {
    ScopeMutex<Mutex> lock(m_mutex);

    if ((std::size_t)index < m_fd_groups.size()) {
        return m_fd_groups[index];
    }

    int new_size = int(index * 1.5);

    for (int i = m_fd_groups.size(); i < new_size; i++) {
        m_fd_groups.push_back(new FdEvent(i));
    }

    return m_fd_groups[index];
}


}

