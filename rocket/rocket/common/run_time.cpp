#include "rocket/common/run_time.h"
#include <string>

namespace rocket {

std::string RunTime::getMsgId() const { return m_msgId; }
std::string RunTime::getMethodName() const { return m_method_name; }
void RunTime::setMsgId(std::string msg_id) { m_msgId = msg_id; }
void RunTime::setMethodName(std::string method_name) {
	m_method_name = method_name;
}
RpcInterface* RunTime::getRpcInterface() const { return m_rpc_interface; }
void RunTime::setRpcInterface(RpcInterface* rpc_interface) {
	m_rpc_interface = rpc_interface;
}

} // namespace rocket