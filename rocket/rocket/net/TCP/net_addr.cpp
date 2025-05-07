#include "rocket/net/TCP/net_addr.h"
#include "rocket/common/log.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

namespace rocket {

IPNetAddr::IPNetAddr(const std::string& ip, uint16_t port)
    : m_ip(ip)
    , m_port(port) {
	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
	m_addr.sin_port = htons(m_port);
}

IPNetAddr::IPNetAddr(const std::string& addr) {
	size_t i = addr.find_first_of(':');
	if (i == std::string::npos) {
		ERRORLOG("invalid ipv4 address [%s]", addr.c_str());
	}

	m_ip = addr.substr(0, i);
	m_port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());

	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
	m_addr.sin_port = htons(m_port);
}

IPNetAddr::IPNetAddr(sockaddr_in addr)
    : m_addr(addr) {
	m_ip = std::string(inet_ntoa(m_addr.sin_addr));
	m_port = ntohs(m_addr.sin_port);
}

sockaddr* IPNetAddr::getSockAddr() {
	return reinterpret_cast<sockaddr*>(&m_addr);
}

socklen_t IPNetAddr::getSockLen() { return sizeof(m_addr); }

int IPNetAddr::getFamily() { return AF_INET; }

std::string IPNetAddr::toString() {
	std::string result = m_ip + ":" + std::to_string(m_port);
	return result;
}

bool IPNetAddr::checkValid() {
	return CheckValid(toString());
}

bool IPNetAddr::CheckValid(const std::string& addr) {
	size_t i = addr.find_first_of(':');
	if (i == std::string::npos) {
		return false;
	}

	std::string ip = addr.substr(0, i);
	std::string port = addr.substr(i + 1, addr.size() - i - 1);
	if (ip.empty() || port.empty()) {
		return false;
	}

	if (std::stoi(port) < 0 || std::stoi(port) > 65535) {
		return false;
	}
	return (inet_addr(ip.c_str()) != INADDR_NONE);
}

} // namespace rocket