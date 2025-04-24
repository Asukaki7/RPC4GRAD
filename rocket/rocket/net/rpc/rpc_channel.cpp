
#include "rocket/net/rpc/rpc_channel.h"
#include "coder/abstract_protocol.h"
#include "coder/tinyPB_protocol.h"
#include "rocket/common/error_code.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/TCP/tcp_server.h"
#include "rpc/rpc_controller.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>

namespace rocket {

RpcChannel::RpcChannel(NetAddr::s_ptr remote_addr)
    : m_remote_addr(remote_addr) {}
RpcChannel::~RpcChannel() {}
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

	// request 序列化
	if (!request->SerializeToString(&req_protocol->getPbBody())) {
		std::string err_info = "failed to serialize request";
		my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
		ERRORLOG("%s | %s, origin request [%s]",
		         req_protocol->getMsgId().c_str(), err_info.c_str(),
		         request->DebugString().c_str());
		return;
	}

	// 发送rpc请求
	TcpClient client(m_remote_addr);

	client.connect([&client, req_protocol, done]() {
		client.writeMessage(req_protocol, [&client, req_protocol,
		                                   done](AbstractProtocol::s_ptr) {
			INFOLOG("[%s] | send request success. call method name [%s]",
			        req_protocol->getMsgId().c_str(),
			        req_protocol->getMethodName().c_str());

			client.readMessage(req_protocol->getMsgId(), [done](
			                                                 AbstractProtocol::
			                                                     s_ptr msg) {
				std::shared_ptr<rocket::TinyPBProtocol> rsp_protocol =
				    std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
				INFOLOG(
				    "[%s] | success get rpc response, call method name [%s]",
				    rsp_protocol->getMsgId().c_str(),
				    rsp_protocol->getMethodName().c_str());

				if (done) {
					done->Run();
				}
			});
		});
	});
}
} // namespace rocket