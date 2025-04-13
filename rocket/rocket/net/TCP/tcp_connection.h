#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include "fd_event.h"
#include "io_thread.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_buffer.h"
#include <memory>

namespace rocket {

enum class TcpState {
	NotConnecetd = 1, // can do io
	Connected = 2, // can do io
	HalfClosing = 3, // server call shutdown, write half close, can read, but cant write;
	Closed = 4, // can't do io
};

class TcpConnection {
public:
	typedef std::shared_ptr<TcpConnection> s_ptr;

public:
	TcpConnection(IOThread* io_thread, int fd, int buffer_size,
	              NetAddr::s_ptr remote_addr);
	~TcpConnection();
	void onRead();

	void execute();

	void onWrite();

	void setState(const TcpState state);

	TcpState getState();

	void clear();

	// 服务器主动关闭连接 
	void shutdown();
private:
	NetAddr::s_ptr m_local_addr;
	NetAddr::s_ptr m_remote_addr;

	TcpBuffer::s_ptr m_in_buffer;  // 接受缓冲区
	TcpBuffer::s_ptr m_out_buffer; // 发送缓冲区

	IOThread* m_io_thread{}; // 代表持有该连接的IO线程

	FdEvent* m_fd_event{};
	int m_fd{};

	TcpState m_state;
};

} // namespace rocket

#endif