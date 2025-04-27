#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/timer_event.h"
#include <functional>
#include <string>
namespace rocket {

class TcpClient {
public:
	typedef std::shared_ptr<TcpClient> s_ptr;

public:
	TcpClient(NetAddr::s_ptr remote_addr);
	~TcpClient();

	// 异步的进行connect 如果connect完成 done函数会被调用（不管成功与否都应该回调）
	void connect(std::function<void()> done);

	// 异步的发送 Message 如果发送成功 done函数会被调用
	// 函数的入参就是Message对象
	void
	writeMessage(rocket::AbstractProtocol::s_ptr message,
	             std::function<void(rocket::AbstractProtocol::s_ptr)> done);

	// 异步的读取 Message 如果读取成功 done函数会被调用
	// 函数的入参就是Message对象
	void readMessage(const std::string& msg_id,
	                 std::function<void(rocket::AbstractProtocol::s_ptr)> done);

	void stop();

	int getConnectErrorCode() const;
	std::string getConnectErrorMsg() const;

	NetAddr::s_ptr getRemoteAddr() const;
	NetAddr::s_ptr getLocalAddr() const;

	void initLocalAddr();

	void addTimerEvent(TimerEvent::s_ptr timer_event);
	
private:
	NetAddr::s_ptr m_remote_addr{nullptr};
	NetAddr::s_ptr m_local_addr{nullptr};
	EventLoop* m_event_loop{nullptr};
	int m_fd{-1};
	FdEvent* m_fd_event{nullptr};

	TcpConnection::s_ptr m_connection{nullptr};

	int m_connect_error_code{0};
	std::string m_connect_error_msg{};
};

} // namespace rocket

#endif