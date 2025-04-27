#ifndef ROCKET_NET_RPC_RPC_CHANNEL_H
#define ROCKET_NET_RPC_RPC_CHANNEL_H

#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/timer_event.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>

namespace rocket {


#define NEWMESSAGE(type, var_name) \
	std::shared_ptr<type> var_name = std::make_shared<type>(); \

#define NEWRPCCONTROLLER(var_name) \
	std::shared_ptr<rocket::RpcController> var_name = \
		std::make_shared<rocket::RpcController>(); \

#define NEWRPCCHANNEL(addr, var_name) \
	std::shared_ptr<rocket::RpcChannel> var_name = \
		std::make_shared<rocket::RpcChannel>(std::make_shared<rocket::IPNetAddr>(addr)); \

#define CALLRPC(addr, method_name, controller, request, response, done) \
{\
	NEWRPCCHANNEL(addr, channel); \
	channel->Init(controller, request, response, done); \
	Order_Stub(channel.get()).method_name(controller.get(), request.get(), response.get(), done.get());\
}\

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
	TimerEvent::s_ptr getTimerEvent() const;

	void resetTimerEvent();
private:
	NetAddr::s_ptr m_remote_addr{nullptr};
	NetAddr::s_ptr m_local_addr{nullptr};

	controller_s_ptr m_controller{nullptr};
	message_s_ptr m_request{nullptr};
	message_s_ptr m_response{nullptr};
	closure_s_ptr m_closure{nullptr};

	bool m_is_init{false};

	TcpClient::s_ptr m_client{nullptr};

	TimerEvent::s_ptr m_timer_event{nullptr};
};

} // namespace rocket

#endif
