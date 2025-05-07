#ifndef ROCKET_NET_RPC_RPC_CONTROLLER_H
#define ROCKET_NET_RPC_RPC_CONTROLLER_H

#include "rocket/net/TCP/net_addr.h"
#include <cstdint>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <string>

namespace rocket {

class RpcController : public google::protobuf::RpcController {
public:
	RpcController();
	~RpcController();
	void Reset();

	bool Failed() const;

	std::string ErrorText() const;

	void StartCancel();

	void SetFailed(const std::string& reason);

	bool IsCanceled() const;

	void NotifyOnCancel(google::protobuf::Closure* callback);

	void SetError(int32_t error_code, const std::string& error_info);

	int32_t GetErrorCode() const;

	std::string GetErrorInfo() const;

	void SetMsgId(const std::string& msg_id);

	std::string GetMsgId() const;
	
	void SetLocalAddr(NetAddr::s_ptr local_addr);

	NetAddr::s_ptr GetLocalAddr() const;

	void SetRemoteAddr(NetAddr::s_ptr remote_addr);

	NetAddr::s_ptr GetRemoteAddr() const;

	void SetTimeout(int timeout);

	int GetTimeout() const;

	// return true if the rpc call is finished
	bool IsFinished() const;

	void SetFinished(bool finished);

private:
	int32_t m_error_code{0};
	std::string m_error_info;
	std::string m_msg_id;
	
	bool m_is_failed{false};
	bool m_is_canceled{false};
	bool m_is_finished{false};

	NetAddr::s_ptr m_local_addr;
	NetAddr::s_ptr m_remote_addr;

	int32_t m_timeout{1000}; // ms
};
} // namespace rocket

#endif
