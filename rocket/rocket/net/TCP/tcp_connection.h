#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_buffer.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace rocket {

enum class TcpState {
	NotConnecetd = 1, // can do io
	Connected = 2,    // can do io
	HalfClosing =
	    3, // server call shutdown, write half close, can read, but cant write;
	Closed = 4, // can't do io
};

enum class TcpConnectionType {
	TcpConnectionByServer = 1, // server,代表跟对端客户端的连接
	TcpConnectionByClient = 2, // client,代表跟对端服务端的连接
};

class TcpConnection {
public:
	typedef std::shared_ptr<TcpConnection> s_ptr;

public:
	TcpConnection(
	    EventLoop* event_loop, int fd, int buffer_size,
	    NetAddr::s_ptr remote_addr,
	    TcpConnectionType type = TcpConnectionType::TcpConnectionByServer);
	~TcpConnection();
	void onRead();

	void execute();

	void onWrite();

	void setState(const TcpState state);

	TcpState getState();

	void clear();

	// 服务器主动关闭连接
	void shutdown();

	void setConnectionType(TcpConnectionType type);

	// 监听可写事件 这里的可写事件是指fd上有数据可以发送
	void listenWrite();

	// 监听可读事件 这里的可读事件是指fd上有数据可读
	void listenRead();

	void pushSendMessage(AbstractProtocol::s_ptr,
	                     std::function<void(rocket::AbstractProtocol::s_ptr)>);

	void pushReadMessage(const std::string& req_id, 
	                     std::function<void(rocket::AbstractProtocol::s_ptr)>);

private:
	NetAddr::s_ptr m_local_addr;
	NetAddr::s_ptr m_remote_addr;

	TcpBuffer::s_ptr m_in_buffer;  // 接受缓冲区
	TcpBuffer::s_ptr m_out_buffer; // 发送缓冲区

	EventLoop* m_event_loop{}; // 代表持有该连接的IO线程

	FdEvent* m_fd_event{};
	int m_fd{};

	TcpState m_state;

	AbstractCoder* m_coder{};

	TcpConnectionType m_connection_type{
	    TcpConnectionType::TcpConnectionByServer};

	// key: req_id value: callback 这里的req_id是唯一标识符
	std::vector<std::pair<AbstractProtocol::s_ptr,
	                      std::function<void(rocket::AbstractProtocol::s_ptr)>>>
	    m_write_done;

	std::map<std::string, std::function<void(rocket::AbstractProtocol::s_ptr)>>
	    m_read_done;
};

} // namespace rocket

#endif