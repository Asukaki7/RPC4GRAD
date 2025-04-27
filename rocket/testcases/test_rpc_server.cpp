#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/TCP/tcp_server.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinyPB_coder.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "testcases/order.pb.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <google/protobuf/service.h>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class OrderImpl : public Order {
public:
	void makeOrder(google::protobuf::RpcController* controller,
	               const ::makeOrderRequest* request,
	               ::makeOrderResponse* response,
	               ::google::protobuf::Closure* done) override {
		DEBUGLOG("make order begin sleep");
		DEBUGLOG("make order end sleep");
		if (request->price() < 10) {
			response->set_ret_code(-1);
			response->set_res_info("short balance");
			return;
		}
		response->set_order_id("20250422");
	}
	~OrderImpl() {}
};

void test_tcp_server() {
	rocket::IPNetAddr::s_ptr addr =
	    std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);

	DEBUGLOG("create addr %s", addr->toString().c_str());

	rocket::TcpServer tcp_server(addr);

	tcp_server.start();
}

int main() {

	rocket::Config::setGlobalConfig("../conf/rocket.xml");

	rocket::Logger::InitGlobalLogger();

	std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
	rocket::RpcDispatcher::getRpcDispatcher()->registerService(service);
	test_tcp_server();
}