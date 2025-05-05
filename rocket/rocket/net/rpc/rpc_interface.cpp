#include "rocket/net/rpc/rpc_interface.h"
#include "rocket/common/log.h"
#include "rocket/net/rpc/rpc_closure.h"
#include "rocket/net/rpc/rpc_controller.h"

namespace rocket {

RpcInterface::RpcInterface(const google::protobuf::Message* req,
                           google::protobuf::Message* rsp, RpcClosure* done,
                           RpcController* controller)
    : m_req(req)
    , m_rsp(rsp)
    , m_done(done)
    , m_controller(controller) {
	INFOLOG("RpcInterface constructor");
}

RpcInterface::~RpcInterface() {
	INFOLOG("RpcInterface destructor");
	reply();
	destroy();
}

void RpcInterface::reply() {
	if (m_done) {
		m_done->Run();
	}
}

void RpcInterface::destroy() {
	if (m_req) {
		delete m_req;
		m_req = nullptr;
	}
	if (m_rsp) {
		delete m_rsp;
		m_rsp = nullptr;
	}
	if (m_done) {
		delete m_done;
		m_done = nullptr;
	}
	if (m_controller) {
		delete m_controller;
		m_controller = nullptr;
	}
}

std::shared_ptr<RpcClosure>
RpcInterface::newRpcClosure(const std::function<void()>& cb) {
	return std::make_shared<RpcClosure>(shared_from_this(), cb);
}

} // namespace rocket
