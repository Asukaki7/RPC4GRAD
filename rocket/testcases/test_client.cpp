#include "coder/tinyPB_protocol.h"
#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/net/TCP/tcp_client.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinyPB_coder.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <gtest/gtest.h>

void test_connect() {
	// 调用connect 连接 server
	// write 一个字符串
	// 等待read返回结果
	auto fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		ERRORLOG("invalid fd [%d]", fd);
		std::exit(1);
	}

	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(12345);
	inet_aton("127.0.0.1", &server_addr.sin_addr);

	int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr),
	                 sizeof(server_addr));

	DEBUGLOG("connect success");

	std::string test_msg = "hello rocket!";

	rt = write(fd, test_msg.c_str(), test_msg.length());

	DEBUGLOG("success write %d bytes, [%s]", rt, test_msg.c_str());

	char buf[100];
	rt = read(fd, buf, 100);

	DEBUGLOG("success read %d bytes, [%s]", rt, buf);
}

void test_tcp_client() {
	rocket::IPNetAddr::s_ptr addr =
	    std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
	rocket::TcpClient client(addr);
	client.connect([addr, &client]() {
		DEBUGLOG("connect to [%s] success", addr->toString().c_str());
		std::shared_ptr<rocket::TinyPBProtocol> message =
		    std::make_shared<rocket::TinyPBProtocol>();
		message->setMsgId("114514");
		message->setMethodName("testConnect.make");
		message->setPbBody("testpbdata");
		client.writeMessage(message,
		                    [](rocket::AbstractProtocol::s_ptr msg_ptr) {
			                    DEBUGLOG("send message success");
		                    });
		client.readMessage(
		    "114514", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
			    std::shared_ptr<rocket::TinyPBProtocol> read_msg =
			        std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
			    DEBUGLOG("MsgId[%s], methodName[%s], get pbBody [%s]",
			             read_msg->getMsgId().c_str(),
			             read_msg->getMethodName().c_str(),
			             read_msg->getPbBody().c_str());
		    });
	});
}

int main() {

	rocket::Config::setGlobalConfig(nullptr);

	rocket::Logger::InitGlobalLogger(0);
	test_tcp_client();
}