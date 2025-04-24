#ifndef ROCKET_NET_RPC_RPC_CHANNEL_H
#define ROCKET_NET_RPC_RPC_CHANNEL_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

namespace rocket {

class RpcChannel : public google::protobuf::RpcChannel {
public:
	RpcChannel(NetAddr::s_ptr remote_addr);
	~RpcChannel();
	void CallMethod(const google::protobuf::MethodDescriptor* method,
	                google::protobuf::RpcController* controller,
	                const google::protobuf::Message* request,
	                google::protobuf::Message* response,
	                google::protobuf::Closure* done);

private:
	NetAddr::s_ptr m_remote_addr{nullptr};
	NetAddr::s_ptr m_local_addr{nullptr};
};

} // namespace rocket

#endif
