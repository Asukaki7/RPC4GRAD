#ifndef ROCKET_NET_ABSTRACT_PROTOCOL_H
#define ROCKET_NET_ABSTRACT_PROTOCOL_H


#include <memory>
#include <string>

namespace rocket {

struct AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol>{

public:
	typedef std::shared_ptr<AbstractProtocol> s_ptr;

	std::string getMsgId() const { return m_msg_id; }
	void setMsgId(const std::string& msg_id) { m_msg_id = msg_id; }

	virtual ~AbstractProtocol() {}
protected:
	std::string m_msg_id; // 请求id
};

} // namespace rocket

#endif