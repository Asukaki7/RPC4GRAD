#ifndef ROCKET_NET_CODER_TINYPB_CODER_H
#define ROCKET_NET_CODER_TINYPB_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include <cstdint>

namespace rocket {

class TinyPBCoder : public AbstractCoder {
public:
	TinyPBCoder() = default;
	~TinyPBCoder() = default;

	void encode(std::vector<AbstractProtocol::s_ptr>& message,
	            TcpBuffer::s_ptr out_buffer);

	void decode(std::vector<AbstractProtocol::s_ptr>& out_message,
	            TcpBuffer::s_ptr buffer);

private:
	const char* encodeTinyPB(TinyPBProtocol::s_ptr message, int32_t& len);
};

} // namespace rocket

#endif