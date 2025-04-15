#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/abstract_protocol.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "tcp_connection.h"
#include <functional>
namespace rocket {

class TcpClient {

public:
	TcpClient(NetAddr::s_ptr remote_addr);
	~TcpClient();

	// 异步的进行connect 如果connect成功 done函数会被调用
	void connect(std::function<void()> done);

	// 异步的发送 Message 如果发送成功 done函数会被调用
	// 函数的入参就是Message对象
	void
	writeMessage(rocket::AbstractProtocol::s_ptr message,
	             std::function<void(rocket::AbstractProtocol::s_ptr)> done);

	// 异步的读取 Message 如果读取成功 done函数会被调用
	// 函数的入参就是Message对象
	void readMessage(rocket::AbstractProtocol::s_ptr message,
	                 std::function<void(rocket::AbstractProtocol::s_ptr)> done);

private:
	NetAddr::s_ptr m_remote_addr{nullptr};
	EventLoop* m_event_loop{nullptr};
	int m_fd{-1};
	FdEvent* m_fd_event{nullptr};

	TcpConnection::s_ptr m_connection{nullptr};
};

} // namespace rocket

#endif