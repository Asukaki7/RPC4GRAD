#ifndef ROCKET_RPC_RPC_INTERFACE_H
#define ROCKET_RPC_RPC_INTERFACE_H

#include "rocket/net/rpc/rpc_controller.h"
#include <functional>
#include <google/protobuf/message.h>
#include <memory.h>
#include <memory>
namespace rocket {

class RpcClosure;

/*
 * Rpc Interface base Class
 * All interface class should inherit from this class
 */

class RpcInterface : public std::enable_shared_from_this<RpcInterface> {
public:
	RpcInterface(const google::protobuf::Message* req,
	             google::protobuf::Message* rsp, RpcClosure* done,
	             RpcController* controller);

	virtual ~RpcInterface();

	// reply to client
	virtual void reply();

	// free resource
	void destroy();

	// alloc a closure object which handle by this interface
	std::shared_ptr<RpcClosure> newRpcClosure(const std::function<void()>& cb);

	// core business deal method
	virtual void run() = 0;
	
	// set error code and error info to response message
	virtual void set_error(int err_code, const std::string& err_info) = 0;

protected:
	const google::protobuf::Message* m_req{nullptr};
	google::protobuf::Message* m_rsp{nullptr};
	RpcClosure* m_done{nullptr}; // callback
	RpcController* m_controller{nullptr};
};

} // namespace rocket

#endif