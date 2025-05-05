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
#include <ostream>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

class OrderImpl : public Order {
public:
	void makeOrder(google::protobuf::RpcController* controller,
	               const ::makeOrderRequest* request,
	               ::makeOrderResponse* response,
	               ::google::protobuf::Closure* done) override {
		APPDEBUGLOG("make order begin sleep");
		
		APPDEBUGLOG("make order end sleep");
		if (request->price() < 10) {
			response->set_ret_code(-1);
			response->set_res_info("short balance");
			return;
		}
		response->set_order_id("20250422");
	}
	~OrderImpl() {}
};

extern rocket::Config* g_config;


int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cout << "start test rpc server error, please input config file"
		          << std::endl;
		std::cout << "example: ./test_rpc_server ../conf/rocket.xml"
		          << std::endl;
	}
	rocket::Config::setGlobalConfig(argv[1]);

	rocket::Logger::InitGlobalLogger();

	std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
	rocket::RpcDispatcher::getRpcDispatcher()->registerService(service);

	rocket::IPNetAddr::s_ptr addr =
	    std::make_shared<rocket::IPNetAddr>("127.0.0.1", rocket::Config::GetGlobalConfig()->m_port);

	rocket::TcpServer tcp_server(addr);

	tcp_server.start();
}