#include "rocket/net/fd_event.h"
#include <cstring>
#include <sys/epoll.h>


namespace rocket {


FdEvent::FdEvent(int fd) : m_fd(fd) {
    memset(&m_listen_event, 0, sizeof(m_listen_event));
}


FdEvent::~FdEvent() {

}


std::function<void()> FdEvent::handler(TriggerEvent event) {
    if (event == TriggerEvent::IN_EVENT) {
        return m_read_callback;
    } else {
        return m_write_callback;
    }
}

void FdEvent::listenRead(TriggerEvent event_type, std::function<void()> callback) {
    if (event_type == TriggerEvent::IN_EVENT) {
        m_listen_event.events |= EPOLLIN;
        m_read_callback = callback;  
    } else {
        m_listen_event.events |= EPOLLOUT;
        m_write_callback = callback;
    }
    m_listen_event.data.ptr = this;
}



} // namespace rocket



