#include "rocket/net/TCP/tcp_connection.h"
#include "coder/tinyPB_protocol.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_buffer.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinyPB_coder.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace rocket {

TcpConnection::TcpConnection(
    EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr remote_addr,
    NetAddr::s_ptr local_addr,
    TcpConnectionType type /* = TcpConnectionByServer*/)
    : m_local_addr(local_addr)
    , m_remote_addr(remote_addr)
    , m_event_loop(event_loop)
    , m_fd(fd)
    , m_state(TcpState::NotConnecetd)
    , m_connection_type(type) {
	m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
	m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

	m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(fd);
	m_fd_event->setNonBlock();

	m_coder = new TinyPBCoder();

	if (m_connection_type == TcpConnectionType::TcpConnectionByServer) {
		listenRead();

	}
}

TcpConnection::~TcpConnection() {
	
	if (m_coder) {
		delete m_coder;
		m_coder = nullptr;
	}
}

void TcpConnection::onRead() {
	// 1. 从socket 缓冲区 调用系统的read函数读取字节 in_buffer 里面

	if (m_state != TcpState::Connected) {
		ERRORLOG("onRead error, client has already disconnected, addr[%s], "
		         "clientfd[%d]",
		         m_remote_addr->toString().c_str(), m_fd);
		clear();
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
		
		INFOLOG("remote close, remote addr [%s], clientfd[%d]",
		        m_remote_addr->toString().c_str(), m_fd);
		clear();
		return;
	}

	if (!is_read_all) {
		ERRORLOG("not read all date");
	}

	
	execute();
}

void TcpConnection::execute() {
	// 将 RPC 请求执行业务逻辑，获取 RPC 响应, 再把 RPC 响应发送回去
	if (m_connection_type == TcpConnectionType::TcpConnectionByServer) {
		std::vector<AbstractProtocol::s_ptr> request_messages;
		
		m_coder->decode(request_messages, m_in_buffer);

		for (size_t i = 0; i < request_messages.size(); i++) {
			// 1. 针对每个请求调用rpc方法，获取响应message
			// 2. 将相应message放入到发送缓冲区，监听可写事件回包
			INFOLOG("success get request [%s] from client[%s]",
			        request_messages[i]->getMsgId().c_str(),
			        m_remote_addr->toString().c_str());
			std::shared_ptr<TinyPBProtocol> response_message =
			    std::make_shared<TinyPBProtocol>();
			// response_message->setPbBody("hello this is rocket rpc test");
			// response_message->setMsgId(result[i]->getMsgId());

			RpcDispatcher::getRpcDispatcher()->dispatch(request_messages[i], response_message, this);
			
		}

	} else if (m_connection_type == TcpConnectionType::TcpConnectionByClient) {
		// 从buffer里decode解码 得到message对象, 执行回调
		std::vector<AbstractProtocol::s_ptr> result;
		m_coder->decode(result, m_in_buffer);

		for (size_t i = 0; i < result.size(); i++) {
			std::string msg_id = result[i]->getMsgId();
			auto it = m_read_done.find(msg_id);
			if (it != m_read_done.end()) {
				it->second(result[i]);
				m_read_done.erase(it);
			}
		}
	}
}

void TcpConnection::onWrite() {
	// 将当前out_buffer 里面的数据全部发送给client

	if (m_state != TcpState::Connected) {
		ERRORLOG("onWrite error, client has already disconnected, addr[%s], "
		         "clientfd[%d]",
		         m_remote_addr->toString().c_str(), m_fd);
		clear();
		return;
	}

	if (m_connection_type == TcpConnectionType::TcpConnectionByClient) {
		// 1. 将数据序列化进行编码 写入到buffer
		// 2. 将数据写入到buffer里面 然后全部发送
		// TODO 考虑粘包？
		std::vector<AbstractProtocol::s_ptr> message;
		for (size_t i = 0; i < m_write_done.size(); i++) {
			message.push_back(m_write_done[i].first);
		}
		m_coder->encode(message, m_out_buffer);
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
		m_event_loop->addEpollEvent(m_fd_event);
	}

	if (m_connection_type == TcpConnectionType::TcpConnectionByClient) {
		for (size_t i = 0; i < m_write_done.size(); i++) {
			m_write_done[i].second(m_write_done[i].first);
		}
	}
	m_write_done.clear();
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

	m_event_loop->deleteEpollEvent(m_fd_event);

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

void TcpConnection::setConnectionType(TcpConnectionType type) {
	m_connection_type = type;
}

void TcpConnection::listenWrite() {
	m_fd_event->listen(FdEvent::OUT_EVENT,
	                   std::bind(&TcpConnection::onWrite, this));
	DEBUGLOG("fd [%d] listenWrite", m_fd);
	m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::listenRead() {
	m_fd_event->listen(FdEvent::IN_EVENT,
	                   std::bind(&TcpConnection::onRead, this));
	DEBUGLOG("fd [%d] listenRead", m_fd);
	m_event_loop->addEpollEvent(m_fd_event);
}

void TcpConnection::pushSendMessage(
    AbstractProtocol::s_ptr message,
    std::function<void(rocket::AbstractProtocol::s_ptr)> done) {
	m_write_done.push_back(std::make_pair(message, done));
}

void TcpConnection::pushReadMessage(
    const std::string& msg_id,
    std::function<void(rocket::AbstractProtocol::s_ptr)> done) {
	m_read_done.insert(std::make_pair(msg_id, done));
}

NetAddr::s_ptr TcpConnection::getLocalAddr() { return m_local_addr; }

NetAddr::s_ptr TcpConnection::getRemoteAddr() { return m_remote_addr; }

void TcpConnection::reply(std::vector<AbstractProtocol::s_ptr>& messages) {
	m_coder->encode(messages, m_out_buffer);
	listenWrite();
}

} // namespace rocket