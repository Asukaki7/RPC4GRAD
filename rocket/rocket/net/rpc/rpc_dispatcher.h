#ifndef ROCKET_RPC_RPC_DISPATCHER_H
#define ROCKET_RPC_RPC_DISPATCHER_H

#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include <google/protobuf/service.h>
#include <map>
#include <memory>
#include <string>

namespace rocket {

class TcpConnection;

class RpcDispatcher {
public:
	typedef std::shared_ptr<google::protobuf::Service> service_sptr;

	void dispatch(AbstractProtocol::s_ptr request,
	              AbstractProtocol::s_ptr response, TcpConnection* connection);
	void registerService(service_sptr service);

	void setTinyPBErrorCode(std::shared_ptr<TinyPBProtocol> msg, int err_code,
	                        const std::string& err_info);

private:
	bool parseServiceFullName(const std::string& full_name,
	                          std::string& service_name,
	                          std::string& method_name);

private:
	std::map<std::string, service_sptr> m_service_map;
};

} // namespace rocket
#endif