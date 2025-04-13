#include "rocket/net/TCP/tcp_server.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/tcp_acceptor.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/io_thread_group.h"
#include <atomic>

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
}
void TcpServer::init() {
	m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);

	m_main_event_loop = EventLoop::getCurrentEventLoop();
	m_io_thread_group = new IOthreadGroup(2);

	m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
	m_listen_fd_event->listen(FdEvent::IN_EVENT,
	                          std::bind(&TcpServer::onAccept, this));
	m_main_event_loop->addEpollEvent(m_listen_fd_event);
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

	// m_io_thread_group->getIOthread()->geteventloop()->addEpollEvent();
    TcpConnection::s_ptr connection = std::make_shared<TcpConnection>(
        m_io_thread_group->getIOthread(), client_fd, 128,
        remote_addr
    );
    connection->setState(TcpState::Connected);

    m_client.insert(connection);
	INFOLOG("TcpServer succ get client, fd = [%d]", client_fd);
}

} // namespace rocket