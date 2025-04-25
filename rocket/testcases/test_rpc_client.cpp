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
#include "rpc/rpc_channel.h"
#include "rpc/rpc_closure.h"
#include "rpc/rpc_controller.h"
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

void test_tcp_client() {
	rocket::IPNetAddr::s_ptr addr =
	    std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
	rocket::TcpClient client(addr);
	client.connect([addr, &client]() {
		DEBUGLOG("connect to [%s] success", addr->toString().c_str());
		std::shared_ptr<rocket::TinyPBProtocol> message =
		    std::make_shared<rocket::TinyPBProtocol>();
		message->setMsgId("99998888");
		message->setPbBody("testpbdata");

		makeOrderRequest request;
		request.set_price(100);
		request.set_good("apple");
        
		if (request.SerializeToString(&message->getPbBody())) {
			DEBUGLOG("serialize success, body [%s]",
			         message->getPbBody().c_str());
		} else {
			DEBUGLOG("serialize failed");
		}

		message->setMethodName("Order.makeOrder");
		client.writeMessage(message,
		                    [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
			                    DEBUGLOG("send message success, request [%s]",
			                             request.DebugString().c_str());
		                    });
		client.readMessage(
		    "99998888", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
			    std::shared_ptr<rocket::TinyPBProtocol> read_msg =
			        std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
			    DEBUGLOG("MsgId[%s], methodName[%s], get pbBody [%s]",
			             read_msg->getMsgId().c_str(),
			             read_msg->getMethodName().c_str(),
			             read_msg->getPbBody().c_str());
			    makeOrderResponse response;
			    if (response.ParseFromString(read_msg->getPbBody())) {
				    DEBUGLOG("parse success, body [%s]",
				             read_msg->getPbBody().c_str());
			    } else {
				    ERRORLOG("parse failed");
				    return;
			    }
			    DEBUGLOG("get response, order_id [%s], ret_code [%d], res_info "
			             "[%s], response [%s]",
			             response.order_id().c_str(), response.ret_code(),
			             response.res_info().c_str(),
			             response.DebugString().c_str());
		    });
	});
}

void test_rpc_channel() {
	rocket::IPNetAddr::s_ptr addr =
	    std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
	std::shared_ptr<rocket::RpcChannel> channel = std::make_shared<rocket::RpcChannel>(addr);
	
	

	std::shared_ptr<makeOrderRequest> request = std::make_shared<makeOrderRequest>();
	request->set_price(100);
	request->set_good("apple");

	std::shared_ptr<makeOrderResponse> response = std::make_shared<makeOrderResponse>();
	std::shared_ptr<rocket::RpcController> controller = std::make_shared<rocket::RpcController>();
	controller->SetMsgId("99998888");

	std::shared_ptr<rocket::RpcClosure> done = std::make_shared<rocket::RpcClosure>([request, response, channel]() mutable {
		DEBUGLOG("request: %s, response: %s", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
		INFOLOG("now exit eventLoop");
		channel->getClient()->stop();
		channel.reset();
	});


	channel->Init(controller, request, response, done);

	Order_Stub stub(channel.get());
	stub.makeOrder(controller.get(), request.get(), response.get(), done.get());

	DEBUGLOG("response: %s", response->DebugString().c_str());
}

int main() {

	rocket::Config::setGlobalConfig("../conf/rocket.xml");

	rocket::Logger::InitGlobalLogger();

	//test_tcp_client();
	test_rpc_channel();
	INFOLOG("test rpc channel end");

	return 0;
}