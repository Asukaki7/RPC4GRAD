#include "rocket/net/TCP/tcp_client.h"
#include "rocket/common/log.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/fd_event_group.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
namespace rocket {

TcpClient::TcpClient(NetAddr::s_ptr remote_addr)
    : m_remote_addr(remote_addr) {
	m_event_loop = EventLoop::getCurrentEventLoop();
	m_fd = socket(remote_addr->getFamily(), SOCK_STREAM, 0);

	if (m_fd < 0) {
		ERRORLOG("TCPclient::TcpClient error, faild to cread fd");
		return;
	}

	m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(m_fd);
	m_fd_event->setNonBlock();

	m_connection = std::make_shared<TcpConnection>(
	    m_event_loop, m_fd, 128, m_remote_addr, nullptr,
	    TcpConnectionType::TcpConnectionByClient);
	m_connection->setConnectionType(TcpConnectionType::TcpConnectionByClient);
}
TcpClient::~TcpClient() {
	DEBUGLOG("TcpClient destructor, addr[%s], clientfd[%d]",
	         m_remote_addr->toString().c_str(), m_fd);
	if (m_fd > 0) {
		close(m_fd);
	}
}
// 异步的进行connect 如果connect成功 done函数会被调用
void TcpClient::connect(std::function<void()> done) {
	int rt = ::connect(m_fd, m_remote_addr->getSockAddr(),
	                   m_remote_addr->getSockLen());

	if (rt == 0) {
		DEBUGLOG("connect [%s] success", m_remote_addr->toString().c_str());
		if (done) {
			done();
		}
	} else if (rt == -1) {
		// 表示连接正在建立
		if (errno == EINPROGRESS) {
			m_fd_event->listen(
			    FdEvent::TriggerEvent::OUT_EVENT, [this, done]() {
				    int error = 0;
				    socklen_t error_len = sizeof(error);
				    getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
				    bool is_connect_succ = false;
				    if (error == 0) {
					    DEBUGLOG("connect [%s] success",
					             m_remote_addr->toString().c_str());
					    is_connect_succ = true;
					    m_connection->setState(TcpState::Connected);
				    } else {
					    ERRORLOG("connect [%s] failed, error = %d",
					             m_remote_addr->toString().c_str(), error);
				    }

				    // 连接完后需要去掉可写事件的监听，不然会一直触发
				    m_fd_event->cancle(FdEvent::TriggerEvent::OUT_EVENT);
				    m_event_loop->addEpollEvent(m_fd_event);
				    // 如果连接成功才会执行回调函数
				    if (is_connect_succ && done) {
					    done();
				    }
			    });
			m_event_loop->addEpollEvent(m_fd_event);

			if (!m_event_loop->isLooping()) {
				m_event_loop->loop();
			}
		} else {
			DEBUGLOG("connect error, errno = %d, error = %s", errno,
			         strerror(errno));
		}
	}
}

// 异步的发送 Message 如果发送成功 done函数会被调用 函数的入参就是Message对象
void TcpClient::writeMessage(
    rocket::AbstractProtocol::s_ptr message,
    std::function<void(rocket::AbstractProtocol::s_ptr)> done) {
	// 1. 把message对象写入到connection的发送缓冲区， done也写入
	// 2. 启动connection 可写事件监听
	m_connection->pushSendMessage(message, done);
	m_connection->listenWrite();
}

// 异步的读取 Message 如果读取成功 done函数会被调用 函数的入参就是Message对象
void TcpClient::readMessage(
    const std::string& msg_id,
    std::function<void(rocket::AbstractProtocol::s_ptr)> done) {
	// 1. 监听可读事件
	// 2. 从buffer里decode解码 得到message对象,
	// 判断其msg_id相等，相等则成功，然后回调
	m_connection->pushReadMessage(msg_id, done);
	m_connection->listenRead();
}

} // namespace rocket