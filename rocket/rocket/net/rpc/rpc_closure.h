#ifndef ROCKET_NET_RPC_CLOSURE_H
#define ROCKET_NET_RPC_CLOSURE_H

#include "rocket/common/exception.h"
#include "rocket/common/log.h"
#include "rocket/common/run_time.h"
#include "rocket/net/rpc/rpc_interface.h"
#include <functional>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>

namespace rocket {

class RpcClosure : public google::protobuf::Closure {
public:
	typedef std::shared_ptr<RpcInterface> interface_sptr;
	RpcClosure(interface_sptr interface, std::function<void()> cb)
	    : m_interface(interface)
	    , m_cb(cb) {
		INFOLOG("RpcClosure constructor");
	}
	~RpcClosure() { INFOLOG("RpcClosure destructor"); }
	void Run() {
		// 更新runtime的RpcInterface，这里在执行cb时候，都会以rpcinterface找到对应接口，实现app日志等
		if (m_interface) {
			RunTime::getRuntime().setRpcInterface(m_interface.get());
		}
		try {
			if (m_cb != nullptr) {
				m_cb();
			}
			if (m_interface) {
				m_interface.reset();
			}
		} catch (RocketException& e) {
			ERRORLOG("RocketException exception[%s], deal handle", e.what());
			e.handle();
			if (m_interface) {
				m_interface->set_error(e.errorCode(), e.errorInfo());
				m_interface.reset();
			}
		} catch (std::exception& e) {
			ERRORLOG("std::exception[%s]", e.what());
			if (m_interface) {
				m_interface->set_error(-1, "unkonwn std::exception");
				m_interface.reset();
			}
		} catch (...) {
			ERRORLOG("Unkonwn exception");
			if (m_interface) {
				m_interface->set_error(-1, "unkonwn exception");
				m_interface.reset();
			}
		}
	}

private:
	interface_sptr m_interface{nullptr};
	std::function<void()> m_cb{nullptr};
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RpcClosure);
};
} // namespace rocket
#endif