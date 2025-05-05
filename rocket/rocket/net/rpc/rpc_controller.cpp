#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/common/log.h"
namespace rocket {


RpcController::RpcController() {
	INFOLOG("RpcController constructor");
}
RpcController::~RpcController() {
	INFOLOG("RpcController destructor");
}

void RpcController::Reset() {

	m_error_code = 0;
	m_error_info.clear();
	m_msg_id.clear();
	m_local_addr.reset();
	m_remote_addr.reset();
	m_is_failed = false;
	m_is_canceled = false;
	m_timeout = 1000;
}

bool RpcController::Failed() const { return m_is_failed; }
void RpcController::SetFailed(const std::string& reason) {
	m_is_failed = true;
	m_error_info = reason;
}

std::string RpcController::ErrorText() const { return m_error_info; }

void RpcController::StartCancel() { m_is_canceled = true; }
bool RpcController::IsCanceled() const { return m_is_canceled; }

void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {
	// 如果rpc没完成 那等到完成之后callback被调用
	// 如果rpc已经完成 那callback被立即调用
    if (callback) {
        callback->Run();
    }
    
}

void RpcController::SetError(int32_t error_code,
                             const std::string& error_info) {
	m_error_code = error_code;
	m_error_info = error_info;
	m_is_failed = true;
}

int32_t RpcController::GetErrorCode() const { return m_error_code; }

std::string RpcController::GetErrorInfo() const { return m_error_info; }

void RpcController::SetMsgId(const std::string& msg_id) { m_msg_id = msg_id; }

std::string RpcController::GetMsgId() const { return m_msg_id; }

void RpcController::SetLocalAddr(NetAddr::s_ptr local_addr) {
	m_local_addr = local_addr;
}

NetAddr::s_ptr RpcController::GetLocalAddr() const { return m_local_addr; }

void RpcController::SetRemoteAddr(NetAddr::s_ptr remote_addr) {
	m_remote_addr = remote_addr;
}

NetAddr::s_ptr RpcController::GetRemoteAddr() const { return m_remote_addr; }

void RpcController::SetTimeout(int timeout) { m_timeout = timeout; }

int RpcController::GetTimeout() const { return m_timeout; }

} // namespace rocket