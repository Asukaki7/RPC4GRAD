#include "rocket/net/TCP/tcp_server.h"
#include "config.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/tcp_acceptor.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/io_thread_group.h"
#include "rocket/net/timer_event.h"
#include <atomic>
#include <memory>

namespace rocket {

TcpServer::TcpServer(NetAddr::s_ptr local_addr)
    : m_local_addr(local_addr) {
	init();
	INFOLOG("rocket TcpServer listen success on [%s]",
	        m_local_addr->toString().c_str());
}

TcpServer::~TcpServer() {
	if (m_main_event_loop) {
		delete m_main_event_loop;
		m_main_event_loop = nullptr;
	}
	if (m_io_thread_group) {
		delete m_io_thread_group;
		m_io_thread_group = nullptr;
	}
	if (m_listen_fd_event) {
		delete m_listen_fd_event;
		m_listen_fd_event = nullptr;
	}
}

void TcpServer::init() {
	m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);

	m_main_event_loop = EventLoop::getCurrentEventLoop();
	m_io_thread_group =
	    new IOthreadGroup(Config::GetGlobalConfig()->m_io_threads);

	m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
	m_listen_fd_event->listen(FdEvent::IN_EVENT,
	                          std::bind(&TcpServer::onAccept, this));
	m_main_event_loop->addEpollEvent(m_listen_fd_event);

	m_clear_client_timer_event = std::make_shared<TimerEvent>(
	    5000, true, std::bind(&TcpServer::clearClientTimer, this));

	m_main_event_loop->addTimerEvent(m_clear_client_timer_event);
}

void TcpServer::start() {
	m_io_thread_group->start();
	m_main_event_loop->loop();
}

void TcpServer::onAccept() {
	auto re = m_acceptor->accept();
	int client_fd = re.first;
	auto remote_addr = re.second;
	// FdEvent client_fd_event(client_fd);
	m_client_counts.fetch_add(1, std::memory_order_relaxed);

	// TODO 把clientFd 添加到任意IO线程中
	IOThread* io_thread = m_io_thread_group->getIOthread();
	TcpConnection::s_ptr connection = std::make_shared<TcpConnection>(
	    io_thread->geteventloop(), client_fd, 128, remote_addr, m_local_addr);
	connection->setState(TcpState::Connected);

	m_client.insert(connection);
	INFOLOG("TcpServer succ get client, fd = [%d]", client_fd);
}

void TcpServer::clearClientTimer() {
	for (auto it = m_client.begin(); it != m_client.end();) {
		if ((*it) != nullptr && (*it).use_count() > 0 && (*it)->getState() == TcpState::Closed) {
			DEBUGLOG("TcpConnection [fd:%d] will delete, state = %d", (*it)->getFd(), (*it)->getState());
			it = m_client.erase(it);
		} else {
			++it;
		}
	}
}

} // namespace rocket