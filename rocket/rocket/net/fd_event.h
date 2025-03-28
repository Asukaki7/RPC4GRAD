#ifndef ROCKET_NET_FD_EVENT_H
#define ROCKET_NET_FD_EVENT_H

#include <functional>
#include <sys/epoll.h>
namespace rocket {
class FdEvent {
public:

    enum TriggerEvent {
        IN_EVENT = EPOLLIN,
        OUT_EVENT = EPOLLOUT,
    };
	FdEvent(int fd);
	~FdEvent();
    std::function<void()> handler(TriggerEvent event_type);

    void listenRead(TriggerEvent event_type, std::function<void()> callback);

    int getFd() const {
        return m_fd;
    }

    epoll_event getEpollEvent() const {
        return m_listen_event;
    }
protected:
	int m_fd{-1};

    epoll_event m_listen_event;
	std::function<void()> m_read_callback;
	std::function<void()> m_write_callback;
};
} // namespace rocket

#endif