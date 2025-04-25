#ifndef ROCKET_NET_RPC_RPC_CHANNEL_H
#define ROCKET_NET_RPC_RPC_CHANNEL_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>

namespace rocket {

class RpcChannel : public google::protobuf::RpcChannel,
                   public std::enable_shared_from_this<RpcChannel> {
public:
	typedef std::shared_ptr<RpcChannel> s_ptr;
	typedef std::shared_ptr<google::protobuf::RpcController> controller_s_ptr;
	typedef std::shared_ptr<google::protobuf::Message> message_s_ptr;
	typedef std::shared_ptr<google::protobuf::Closure> closure_s_ptr;

public:
	RpcChannel(NetAddr::s_ptr remote_addr);
	~RpcChannel();

	void Init(controller_s_ptr controller, message_s_ptr request,
	          message_s_ptr response, closure_s_ptr done);

	void CallMethod(const google::protobuf::MethodDescriptor* method,
	                google::protobuf::RpcController* controller,
	                const google::protobuf::Message* request,
	                google::protobuf::Message* response,
	                google::protobuf::Closure* done);


	google::protobuf::RpcController* getController() const;
	google::protobuf::Message* getRequest() const;
	google::protobuf::Message* getResponse() const;
	google::protobuf::Closure* getClosure() const;
	TcpClient* getClient() const;

private:
	NetAddr::s_ptr m_remote_addr{nullptr};
	NetAddr::s_ptr m_local_addr{nullptr};

	controller_s_ptr m_controller{nullptr};
	message_s_ptr m_request{nullptr};
	message_s_ptr m_response{nullptr};
	closure_s_ptr m_closure{nullptr};

	bool m_is_init{false};

	TcpClient::s_ptr m_client{nullptr};
};

} // namespace rocket

#endif
