#ifndef ROCKET_NET_STRING_CODER_H
#define ROCKET_NET_STRING_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/TCP/tcp_buffer.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace rocket {

class stringProtocol : public AbstractProtocol {

public:
	std::string info;
};
class stringCoder : public AbstractCoder {

	// 编码 将messsage对象转换为字节流 写入到buffer
	void encode(std::vector<AbstractProtocol::s_ptr>& message,
	            TcpBuffer::s_ptr out_buffer) {
		for (size_t i = 0; i < message.size(); i++) {
			std::shared_ptr<stringProtocol> msg = std::dynamic_pointer_cast<stringProtocol>(message[i]);
			out_buffer->writeToBuffer(msg->info.c_str(), msg->info.length());
		}
	}

	// 解码 将buffer中的字节流转换为message对象
	void decode(std::vector<AbstractProtocol::s_ptr>& out_message,
	            TcpBuffer::s_ptr buffer) {
		std::vector<char> re;
		buffer->readFromBuffer(re, buffer->readAble());
		std::string info;
		for (size_t i = 0; i < re.size(); i++) {
			info += re[i];
		}
		std::shared_ptr<stringProtocol> msg = std::make_shared<stringProtocol>();
		msg->info = info;
		msg->setMsgId("114514");
		out_message.push_back(msg);
	}
};

} // namespace rocket
#endif