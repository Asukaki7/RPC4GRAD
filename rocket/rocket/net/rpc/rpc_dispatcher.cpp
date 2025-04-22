#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/common/error_code.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/tcp_connection.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include "rocket/net/rpc/rpc_controller.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <string>

namespace rocket {

void RpcDispatcher::dispatch(AbstractProtocol::s_ptr request,
                             AbstractProtocol::s_ptr response,
                             TcpConnection* connection) {
	std::shared_ptr<TinyPBProtocol> request_protocol =
	    std::dynamic_pointer_cast<TinyPBProtocol>(request);
	std::shared_ptr<TinyPBProtocol> response_protocol =
	    std::dynamic_pointer_cast<TinyPBProtocol>(response);
	std::string method_full_name = request_protocol->getMethodName();

	response_protocol->setReqId(request_protocol->getReqId());
	response_protocol->setMethodName(request_protocol->getMethodName());

	std::string service_name{};
	std::string method_name{};
	if (parseServiceFullName(method_full_name, service_name, method_name)) {
		ERRORLOG(
		    "req_id [%s] | parse service name error, method_full_name : [%s]",
		    request_protocol->getReqId().c_str(), method_full_name.c_str());
		setTinyPBErrorCode(response_protocol, ERROR_PARSE_SERVICE_NAME,
		                   "parse service name error");
		return;
	}

	auto it = m_service_map.find(service_name);
	if (it == m_service_map.end()) {
		ERRORLOG("req_id [%s] | service not found, service_name : [%s]",
		         request_protocol->getReqId().c_str(), service_name.c_str());
		setTinyPBErrorCode(response_protocol, ERROR_SERVICE_NOT_FOUND,
		                   "service not found");
		return;
	}

	service_sptr service = it->second;
	const google::protobuf::MethodDescriptor* method =
	    service->GetDescriptor()->FindMethodByName(method_name);
	if (method == nullptr) {
		ERRORLOG("req_id [%s] | method not found, method_name : [%s] in "
		         "service [%s]",
		         request_protocol->getReqId().c_str(), method_name.c_str(),
		         service_name.c_str());
		setTinyPBErrorCode(response_protocol, ERROR_METHOD_NOT_FOUND,
		                   "method not found");
		return;
	}

	google::protobuf::Message* req_msg =
	    service->GetRequestPrototype(method).New();
	// 反序列化，将pb_data 反序列化为req_msg
	if (req_msg->ParseFromString(request_protocol->getPbBody())) {
		ERRORLOG("req_id [%s] | deserialize request error, pb body : [%s]",
		         request_protocol->getReqId().c_str(),
		         request_protocol->getPbBody().c_str());
		setTinyPBErrorCode(response_protocol, ERROR_FAILED_DESERIALIZE,
		                   "deserialize request error");
		if (req_msg != nullptr) {
			delete req_msg;
			req_msg = nullptr;
		}
		return;
	}

	INFOLOG("req_id [%s] | get rpc request[%s]",
	        request_protocol->getReqId().c_str(),
	        req_msg->ShortDebugString().c_str());

	google::protobuf::Message* rsp_msg =
	    service->GetResponsePrototype(method).New();

	RpcController rpc_controller;

	rpc_controller.SetLocalAddr(connection->getLocalAddr());
	rpc_controller.SetRemoteAddr(connection->getRemoteAddr());
	rpc_controller.SetReqId(request_protocol->getReqId());

	service->CallMethod(method, &rpc_controller, req_msg, rsp_msg, nullptr);

	if (!rsp_msg->SerializeToString(&(response_protocol->getPbBody()))) {
		ERRORLOG("req_id [%s] | serialize response error, origin message [%s]",
		         request_protocol->getReqId().c_str(),
		         rsp_msg->ShortDebugString().c_str());
		setTinyPBErrorCode(response_protocol, ERROR_FAILED_SERIALIZE,
		                   "serialize response error");
		if (rsp_msg != nullptr) {
			delete rsp_msg;
			rsp_msg = nullptr;
		}
		if (req_msg != nullptr) {
			delete req_msg;
			req_msg = nullptr;
		}
		return;
	}

	response_protocol->setErrCode(0);

	INFOLOG("%s | dispatch success, request[%s], response[%s]",
	        request_protocol->getReqId().c_str(),
	        req_msg->ShortDebugString().c_str(),
	        rsp_msg->ShortDebugString().c_str());
	if (rsp_msg != nullptr) {
		delete rsp_msg;
		rsp_msg = nullptr;
	}
	if (req_msg != nullptr) {
		delete req_msg;
		req_msg = nullptr;
	}
}

void RpcDispatcher::registerService(service_sptr service) {
	std::string service_name = service->GetDescriptor()->full_name();
	m_service_map[service_name] = service;
}

bool RpcDispatcher::parseServiceFullName(const std::string& full_name,
                                         std::string& service_name,
                                         std::string& method_name) {
	if (full_name.empty()) {
		ERRORLOG("full name is empty!");
		return false;
	}
	size_t pos = full_name.find_last_of('.');
	if (pos == std::string::npos) {
		ERRORLOG("not find [.] in full name : [%s]", full_name.c_str());
		return false;
	}
	service_name = full_name.substr(0, pos);
	method_name = full_name.substr(pos + 1);
	INFOLOG("service_name : [%s], method_name : [%s]", service_name.c_str(),
	        method_name.c_str());
	return true;
}
void RpcDispatcher::setTinyPBErrorCode(std::shared_ptr<TinyPBProtocol> msg,
                                       int err_code,
                                       const std::string& err_info) {
	msg->setErrCode(err_code);
	msg->setErrInfo(err_info);
	msg->setErrInfoLen(err_info.length());
}

} // namespace rocket