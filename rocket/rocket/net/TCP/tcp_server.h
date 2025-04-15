#ifndef ROCKET_NET_TCP_TCP_SERVER_H
#define ROCKET_NET_TCP_TCP_SERVER_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_acceptor.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/io_thread_group.h"
#include <atomic>
#include <set>

namespace rocket {

class TcpServer { // 全局单例对象
public:
	TcpServer(NetAddr::s_ptr local_addr);
	~TcpServer();

	void start();

private:
	void init();

	// 当有新client连接之后执行
	void onAccept();

private:
	TcpAcceptor::s_ptr m_acceptor;
	NetAddr::s_ptr m_local_addr;        // 本地监听地址
	EventLoop* m_main_event_loop{};     // mainReactor 负责链接建立
	IOthreadGroup* m_io_thread_group{}; // subReactor组 负责IO事件处理

	FdEvent* m_listen_fd_event{};

	std::atomic<int> m_client_counts{};

	std::set<TcpConnection::s_ptr> m_client;
};

} // namespace rocket

#endif