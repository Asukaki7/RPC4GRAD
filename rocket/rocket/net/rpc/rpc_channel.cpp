#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include "rocket/common/error_code.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/TCP/tcp_server.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/timer_event.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>

namespace rocket {

RpcChannel::RpcChannel(NetAddr::s_ptr remote_addr)
    : m_remote_addr(remote_addr) {
	m_client = std::make_shared<TcpClient>(m_remote_addr);
}

RpcChannel::~RpcChannel() { INFOLOG("RpcChannel::~RpcChannel()"); }

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
	std::shared_ptr<rocket::TinyPBProtocol> req_protocol =
	    std::make_shared<rocket::TinyPBProtocol>();

	RpcController* my_controller = dynamic_cast<RpcController*>(controller);
	if (my_controller == nullptr) {
		ERRORLOG("failed callmethod, RpcController convert error");
		return;
	}

	if (my_controller->GetMsgId().empty()) {
		req_protocol->setMsgId(MsgUtil::generateMsgId());
		my_controller->SetMsgId(req_protocol->getMsgId());
	} else {
		req_protocol->setMsgId(my_controller->GetMsgId());
	}

	req_protocol->setMethodName(method->full_name());
	INFOLOG("[%s] | callmethod name [%s]", req_protocol->getMsgId().c_str(),
	        req_protocol->getMethodName().c_str());

	if (!m_is_init) {
		std::string err_info = "RpcChannel not inited";
		my_controller->SetError(ERROR_RPC_CHANNEL_NOT_INIT, err_info);
		ERRORLOG("[%s] | %s, rpc channel not inited",
		         req_protocol->getMsgId().c_str(), err_info.c_str());
		return;
	}

	// 1. request 序列化
	if (!request->SerializeToString(&req_protocol->getPbBody())) {
		std::string err_info = "failed to serialize request";
		my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
		ERRORLOG("%s | %s, origin request [%s]",
		         req_protocol->getMsgId().c_str(), err_info.c_str(),
		         request->DebugString().c_str());
		return;
	}

	s_ptr channel = shared_from_this();


	m_timer_event = std::make_shared<TimerEvent>(my_controller->GetTimeout(), false, [my_controller, channel]() mutable {
		my_controller->StartCancel();
		my_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(my_controller->GetTimeout()) + "ms");
		ERRORLOG("[%s] | rpc call timeout, error code [%d], error info [%s]", my_controller->GetMsgId().c_str(), my_controller->GetErrorCode(), my_controller->GetErrorInfo().c_str());

		if (channel->getClosure() != nullptr) {
			channel->getClosure()->Run();
		}
		channel.reset();
	});

	m_client->addTimerEvent(m_timer_event);
	// 2. 发送rpc请求

	m_client->connect([req_protocol, channel]() mutable {
		auto my_controller =
		    dynamic_cast<RpcController*>(channel->getController());
		if (my_controller->IsCanceled()) {
			channel->getTimerEvent()->setCanceled(true);
			return;
		}
		if (channel->m_client->getConnectErrorCode() != 0) {
			my_controller->SetError(channel->m_client->getConnectErrorCode(),
			                        channel->m_client->getConnectErrorMsg());
			ERRORLOG("%s | connect error, error code[%d], error info[%s], peer "
			         "addr[%s]",
			         req_protocol->getMsgId().c_str(),
			         my_controller->GetErrorCode(),
			         my_controller->GetErrorInfo().c_str(),
			         channel->m_client->getRemoteAddr()->toString().c_str());
			
			// 取消定时任务
			channel->getTimerEvent()->setCanceled(true);
			if (channel->getClosure() != nullptr) {
				channel->getClosure()->Run();
			}
			channel.reset();
			return;
		}

		INFOLOG("[%s] | connect success, send request, remote addr [%s], local "
		        "addr [%s]",
		        req_protocol->getMsgId().c_str(),
		        channel->m_client->getRemoteAddr()->toString().c_str(),
		        channel->m_client->getLocalAddr()->toString().c_str());

		channel->m_client->writeMessage(req_protocol, [channel, req_protocol,
		                                               my_controller](
		                                                  AbstractProtocol::
		                                                      s_ptr) mutable {
			INFOLOG("[%s] | send request success. call method name [%s]",
			        req_protocol->getMsgId().c_str(),
			        req_protocol->getMethodName().c_str());

			channel->m_client->readMessage(
			    req_protocol->getMsgId(),
			    [channel, my_controller](AbstractProtocol::s_ptr msg) mutable {
				    std::shared_ptr<rocket::TinyPBProtocol> rsp_protocol =
				        std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
				    INFOLOG(
				        "[%s] | success get rpc response, call method "
				        "name [%s], remote addr [%s], local addr [%s]",
				        rsp_protocol->getMsgId().c_str(),
				        rsp_protocol->getMethodName().c_str(),
				        channel->m_client->getRemoteAddr()->toString().c_str(),
				        channel->m_client->getLocalAddr()->toString().c_str());

					// 当成功读取到回包后，取消定时任务
					channel->getTimerEvent()->setCanceled(true);
					channel->resetTimerEvent();
				    // 3. 反序列化response
				    if (!(channel->getResponse()->ParseFromString(
				            rsp_protocol->getPbBody()))) {
					    ERRORLOG(
					        "[%s] | failed deserialize response, call method "
					        "name [%s]",
					        rsp_protocol->getMsgId().c_str(),
					        rsp_protocol->getMethodName().c_str());
					    my_controller->SetError(ERROR_FAILED_DESERIALIZE,
					                            "failed deserialize response");
					    channel.reset();
					    return;
				    }

				    if (rsp_protocol->getErrCode() != 0) {
					    ERRORLOG("[%s] | call rpc method [%s] failed, error "
					             "code [%d], error info [%s]",
					             rsp_protocol->getMsgId().c_str(),
					             rsp_protocol->getMethodName().c_str(),
					             rsp_protocol->getErrCode(),
					             rsp_protocol->getErrInfo().c_str());
					    my_controller->SetError(rsp_protocol->getErrCode(),
					                            rsp_protocol->getErrInfo());
					    channel.reset();
					    return;
				    }

				    INFOLOG(
				        "[%s] | call rpc method [%s] success, response [%s]",
				        rsp_protocol->getMsgId().c_str(),
				        rsp_protocol->getMethodName().c_str(),
				        channel->getResponse()->ShortDebugString().c_str());
					
					
					
				    // 4. 执行回调函数
				    if (channel->getClosure() != nullptr) {
					    channel->getClosure()->Run();
				    }
				    channel.reset();
			    });
		});
	});
}

void RpcChannel::Init(controller_s_ptr controller, message_s_ptr request,
                      message_s_ptr response, closure_s_ptr closure) {

	if (!m_is_init) {
		m_controller = controller;
		m_request = request;
		m_response = response;
		m_closure = closure;
		m_is_init = true;
	} else {
		ERRORLOG("RpcChannel already inited");
		return;
	}
}

google::protobuf::RpcController* RpcChannel::getController() const {
	return m_controller.get();
}
google::protobuf::Message* RpcChannel::getRequest() const {
	return m_request.get();
}
google::protobuf::Message* RpcChannel::getResponse() const {
	return m_response.get();
}
google::protobuf::Closure* RpcChannel::getClosure() const {
	return m_closure.get();
}
TcpClient* RpcChannel::getClient() const { return m_client.get(); }
TimerEvent::s_ptr RpcChannel::getTimerEvent() const { return m_timer_event; }
void RpcChannel::resetTimerEvent() { m_timer_event = nullptr; }
} // namespace rocket