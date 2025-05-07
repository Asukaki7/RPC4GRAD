#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/common/config.h"
#include "rocket/common/error_code.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/common/run_time.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinyPB_protocol.h"
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

void RpcChannel::callBack() {
	INFOLOG("RpcChannel::callBack()");
	RpcController* my_controller =
	    dynamic_cast<RpcController*>(m_controller.get());
	if (my_controller->IsFinished()) {
		return;
	}

	if (m_closure) {
		m_closure->Run();
		if (my_controller) {
			my_controller->SetFinished(true);
		}
	}
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
	std::shared_ptr<rocket::TinyPBProtocol> req_protocol =
	    std::make_shared<rocket::TinyPBProtocol>();

	RpcController* my_controller = dynamic_cast<RpcController*>(controller);
	if (my_controller == nullptr || request == nullptr || response == nullptr ||
	    done == nullptr) {
		ERRORLOG("failed callmethod, RpcController convert error");
		my_controller->SetError(ERROR_RPC_CHANNEL_INIT_FAILED,
		                        "controller or request or response NULL");
		callBack();
		return;
	}

	if (m_remote_addr == nullptr) {
		ERRORLOG("failed get peer addr");
		my_controller->SetError(ERROR_RPC_CHANNEL_INIT_FAILED,
		                        "failed get peer addr");
		callBack();
		return;
	}

	m_client = std::make_shared<TcpClient>(m_remote_addr);

	if (my_controller->GetMsgId().empty()) {
		// 先从 runtime 里面取, 取不到再生成一个
		// 这样的目的是为了实现 msg_id 的透传，假设服务 A 调用了 B，那么同一个
		// msgid 可以在服务 A 和 B 之间串起来，方便日志追踪
		auto msg_id = RunTime::getRuntime().getMsgId();
		if (!msg_id.empty()) {
			req_protocol->setMsgId(msg_id);
			my_controller->SetMsgId(msg_id);
		} else {
			req_protocol->setMsgId(MsgUtil::generateMsgId());
			my_controller->SetMsgId(req_protocol->getMsgId());
		}
	} else {
		// 如果msg_id不为空，则使用msg_id
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
		callBack();
		return;
	}

	// 1. request 序列化
	if (!request->SerializeToString(&req_protocol->getPbBody())) {
		std::string err_info = "failed to serialize request";
		my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
		ERRORLOG("%s | %s, origin request [%s]",
		         req_protocol->getMsgId().c_str(), err_info.c_str(),
		         request->DebugString().c_str());
		callBack();
		return;
	}

	s_ptr channel = shared_from_this();

	TimerEvent::s_ptr timer_event = std::make_shared<TimerEvent>(
	    my_controller->GetTimeout(), false, [my_controller, channel]() mutable {
		    INFOLOG("[%s] | rpc call timeout, error code [%d], error info [%s]",
		            my_controller->GetMsgId().c_str(),
		            my_controller->GetErrorCode(),
		            my_controller->GetErrorInfo().c_str());
		    if (my_controller->IsFinished()) {
			    channel.reset();
			    return;
		    }
		    my_controller->StartCancel();
		    my_controller->SetError(
		        ERROR_RPC_CALL_TIMEOUT,
		        "rpc call timeout " +
		            std::to_string(my_controller->GetTimeout()) + "ms");
		    ERRORLOG(
		        "[%s] | rpc call timeout, error code [%d], error info [%s]",
		        my_controller->GetMsgId().c_str(),
		        my_controller->GetErrorCode(),
		        my_controller->GetErrorInfo().c_str());

		    channel->callBack();

		    channel.reset();
	    });

	m_client->addTimerEvent(timer_event);
	// 2. 发送rpc请求

	m_client->connect([req_protocol, this]() mutable {
		auto my_controller = dynamic_cast<RpcController*>(getController());
		
		if (getTcpClient()->getConnectErrorCode() != 0) {
			my_controller->SetError(getTcpClient()->getConnectErrorCode(),
			                        getTcpClient()->getConnectErrorMsg());
			ERRORLOG("%s | connect error, error code[%d], error info[%s], peer "
			         "addr[%s]",
			         req_protocol->getMsgId().c_str(),
			         my_controller->GetErrorCode(),
			         my_controller->GetErrorInfo().c_str(),
			         getTcpClient()->getRemoteAddr()->toString().c_str());

			callBack();
			return;
		}

		INFOLOG("[%s] | connect success, send request, remote addr [%s], local "
		        "addr [%s]",
		        req_protocol->getMsgId().c_str(),
		        getTcpClient()->getRemoteAddr()->toString().c_str(),
		        getTcpClient()->getLocalAddr()->toString().c_str());

		getTcpClient()->writeMessage(req_protocol, [this, req_protocol,
		                                            my_controller](
		                                               AbstractProtocol::
		                                                   s_ptr) mutable {
			INFOLOG("[%s] | send request success. call method name [%s]",
			        req_protocol->getMsgId().c_str(),
			        req_protocol->getMethodName().c_str());

			getTcpClient()->readMessage(
			    req_protocol->getMsgId(),
			    [this, my_controller](AbstractProtocol::s_ptr msg) mutable {
				    std::shared_ptr<rocket::TinyPBProtocol> rsp_protocol =
				        std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
				    INFOLOG("[%s] | success get rpc response, call method "
				            "name [%s], remote addr [%s], local addr [%s]",
				            rsp_protocol->getMsgId().c_str(),
				            rsp_protocol->getMethodName().c_str(),
				            getTcpClient()->getRemoteAddr()->toString().c_str(),
				            getTcpClient()->getLocalAddr()->toString().c_str());

				    // 3. 反序列化response
				    if (!(getResponse()->ParseFromString(
				            rsp_protocol->getPbBody()))) {
					    ERRORLOG(
					        "[%s] | failed deserialize response, call method "
					        "name [%s]",
					        rsp_protocol->getMsgId().c_str(),
					        rsp_protocol->getMethodName().c_str());
					    my_controller->SetError(ERROR_FAILED_DESERIALIZE,
					                            "failed deserialize response");
					    callBack();
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
					    callBack();
					    return;
				    }

				    INFOLOG("%s | call rpc success, call method name[%s], peer "
				            "addr[%s], local addr[%s]",
				            rsp_protocol->getMsgId().c_str(),
				            rsp_protocol->getMethodName().c_str(),
				            getTcpClient()->getRemoteAddr()->toString().c_str(),
				            getTcpClient()->getLocalAddr()->toString().c_str());

				    // 4. 执行回调函数
				    callBack();
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
TcpClient* RpcChannel::getTcpClient() const { return m_client.get(); }


NetAddr::s_ptr RpcChannel::FindAddr(const std::string& str) {
	if (IPNetAddr::CheckValid(str)) {
		return std::make_shared<IPNetAddr>(str);
	} else {
		auto it = Config::GetGlobalConfig()->m_rpc_stubs.find(str);
		if (it != Config::GetGlobalConfig()->m_rpc_stubs.end()) {
			INFOLOG("find addr [%s] in global config of str [%s]",
			        it->second.addr->toString().c_str(), str.c_str());
			return it->second.addr;
		} else {
			ERRORLOG("not find addr in global config of str [%s]", str.c_str());
			return nullptr;
		}
	}
	return nullptr;
}

} // namespace rocket