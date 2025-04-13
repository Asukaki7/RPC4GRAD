#include "rocket/net/TCP/tcp_connection.h"
#include "fd_event.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_buffer.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/io_thread.h"
#include <functional>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace rocket {

TcpConnection::TcpConnection(IOThread* io_thread, int fd, int buffer_size,
                             NetAddr::s_ptr remote_addr)
    : m_remote_addr(remote_addr)
    , m_io_thread(io_thread)
    , m_fd(fd)
    , m_state(TcpState::NotConnecetd) {
	m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
	m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

	m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(fd);
	m_fd_event->setNonBlock();
	m_fd_event->listen(FdEvent::IN_EVENT,
	                   std::bind(&TcpConnection::onRead, this));

	io_thread->geteventloop()->addEpollEvent(m_fd_event);
}
TcpConnection::~TcpConnection() {
	DEBUGLOG("TcpConnection destructor, addr[%s], clientfd[%d]",
	         m_local_addr->toString().c_str(), m_fd);
}

void TcpConnection::onRead() {
	// 1. 从socket 缓冲区 调用系统的read函数读取字节 in_buffer 里面

	if (m_state != TcpState::Connected) {
		ERRORLOG("onRead error, client has already disconnected, addr[%s], "
		         "clientfd[%d]",
		         m_remote_addr->toString().c_str(), m_fd);
		return;
	}

	bool is_read_all = false;
	bool is_close = false;

	while (!is_read_all) {
		if (m_in_buffer->writeAble() == 0) {
			m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
		}
		int read_count = m_in_buffer->writeAble();
		int write_index = m_in_buffer->getWriteIndex();

		int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
		DEBUGLOG("success read %d bytes from addr[%s], clientfd[%d]", rt,
		         m_remote_addr->toString().c_str(), m_fd);
		if (rt > 0) {
			m_in_buffer->moveWriteIndex(rt);

			if (rt == read_count) {
				continue;
			} else if (rt < read_count) {
				is_read_all = true;
				break;
			}
		} else if (rt == 0) {
			is_close = true;
			break;
		} else if (rt == -1 && errno == EAGAIN) {
			is_read_all = true;
			break;
		}
	}

	if (is_close) {
		// TODO 处理关闭链接
		INFOLOG("remote close, remote addr [%s], clientfd[%d]",
		        m_remote_addr->toString().c_str(), m_fd);
		clear(); 
		return;
	}

	if (!is_read_all) {
		ERRORLOG("not read all date");
	}

	// 简单的echo，后边补充RPC协议解析！
	execute();
}

void TcpConnection::execute() {
	// 将 RPC 请求执行业务逻辑，获取 RPC 响应, 再把 RPC 响应发送回去

	std::vector<char> tmp;
	int size = m_in_buffer->readAble();
	tmp.resize(size);
	m_in_buffer->readFromBuffer(tmp, size);

	std::string tmp_msg;
	for (int i = 0; i < size; i++) {
		tmp_msg.push_back(tmp[i]);
	}

	INFOLOG("success get request [%s] from client[%s]", tmp_msg.c_str(),
	        m_remote_addr->toString().c_str());

	m_out_buffer->writeToBuffer(tmp_msg.c_str(), tmp_msg.length());

	m_fd_event->listen(FdEvent::OUT_EVENT,
	                   std::bind(&TcpConnection::onWrite, this));

	m_io_thread->geteventloop()->addEpollEvent(m_fd_event);
}

void TcpConnection::onWrite() {
	// 将当前out_buffer 里面的数据全部发送给client

	if (m_state != TcpState::Connected) {
		ERRORLOG("onWrite error, client has already disconnected, addr[%s], "
		         "clientfd[%d]",
		         m_remote_addr->toString().c_str(), m_fd);
		return;
	}

	bool is_write_all = false;

	while (1) {
		if (m_out_buffer->readAble() == 0) {
			DEBUGLOG("no data need to seed to clinet [%s]",
			         m_remote_addr->toString().c_str());
			is_write_all = true;
			break;
		}
		int write_count = m_out_buffer->readAble();
		int read_index = m_out_buffer->getReadIndex();

		int rt =
		    write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_count);

		if (rt >= write_count) {
			DEBUGLOG("no data need to send to client [%s]",
			         m_remote_addr->toString().c_str());
			is_write_all = true;
			break;
		} else if (rt == -1 && errno == EAGAIN) {
			// 发送缓冲区已满 不能再发送
			// this situation， 我们等下次fd可写的时候再次发送数据即可
			ERRORLOG("write data error , errno = EAGIN and rt == -1");
			break;
		}
	}

	if (is_write_all) {
		m_fd_event->cancle(FdEvent::OUT_EVENT);
		m_io_thread->geteventloop()->addEpollEvent(m_fd_event);
	}
}

void TcpConnection::setState(const TcpState state) {
	m_state = TcpState::Connected;
}

TcpState TcpConnection::getState() { return m_state; }

// 处理一些关闭连接后的清理动作
void TcpConnection::clear() {
	if (m_state == TcpState::Closed) {
		return;
	}

	m_fd_event->cancle(FdEvent::IN_EVENT);
	m_fd_event->cancle(FdEvent::OUT_EVENT);

	m_io_thread->geteventloop()->deleteEpollEvent(m_fd_event);

	m_state = TcpState::Closed;
}

void TcpConnection::shutdown() {
	if (m_state == TcpState::Closed || m_state == TcpState::NotConnecetd) {
		return;
	}

	m_state = TcpState::HalfClosing;

	// 调用shutdown关闭读写 意味着服务器不会再对fd进行读写
	// （仅仅只保持单端的）但是客户端还是可以继续读写 发送FIN报文，
	// 触发四次挥手的第一个阶段 服务器进入time_wait阶段
	// 当fd发生可读事件，但是可读的数据为0，即对端发送了FIN
	::shutdown(m_fd, SHUT_RDWR);
}

} // namespace rocket