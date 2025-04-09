#include "rocket/net/TCP/tcp_acceptor.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>

namespace rocket {
TcpAcceptor::TcpAcceptor(NetAddr::s_ptr local_addr)
    : m_local_addr(local_addr) {
	if (!local_addr->checkValid()) {
		ERRORLOG("invalid local addr %s", local_addr->toString().c_str());
		std::exit(1);
	}

	m_family = local_addr->getFamily();

	m_listenfd = socket(m_family, SOCK_STREAM, 0);
	if (m_listenfd < 0) {
		ERRORLOG("create socket error");
		std::exit(1);
	}

	// 设置为非阻塞
	int val = 1;
	if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) !=
	    0) {
		ERRORLOG("setsockopt REUSEADDR error, errno = %d, error = %s", errno,
		         strerror(errno));
	}

	socklen_t len = m_local_addr->getSockLen();

	if (bind(m_listenfd, m_local_addr->getSockAddr(), len) != 0) {
		ERRORLOG("bind error, errno = %d, error = %s", errno, strerror(errno));
		std::exit(1);
	}

	if (listen(m_listenfd, 1024) != 0) {
		ERRORLOG("listen error, errno = %d, error = %s", errno,
		         strerror(errno));
		std::exit(1);
	}
}

TcpAcceptor::~TcpAcceptor() {}

int TcpAcceptor::accept() {
	if (m_family == AF_INET) {
		sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(client_addr));

		socklen_t clien_addr_len = sizeof(client_addr);

		int client_fd = ::accept(m_listenfd, reinterpret_cast<sockaddr*>(&client_addr),
		                  &clien_addr_len);

		if (client_fd < 0) {
            ERRORLOG("accept error, errno = %d, error = %s", errno, strerror(errno));
		}
        IPNetAddr peer_addr(client_addr);
        INFOLOG("A client have accepted success, peer addr [%s]", peer_addr.toString().c_str());

        return client_fd;
	} else {
        // TODO ipv6
    }
    return -1;
}
} // namespace rocket