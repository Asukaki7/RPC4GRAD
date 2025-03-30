#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/eventLoop.h"
#include "rocket/net/fd_event.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "rocket/net/timer_event.h"

int main() {

	rocket::Config::setGlobalConfig("../conf/rocket.xml");

	rocket::Logger::InitGlobalLogger();

	rocket::EventLoop* eventLoop = new rocket::EventLoop();

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd == -1) {
		ERRORLOG("listenfd = -1");
		exit(0);
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_port = htons(12345);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_aton("127.0.0.1", &addr.sin_addr);

	auto rt = bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

	if (rt != 0) {
		ERRORLOG("bind error");
		exit(1);
	}
	rt = listen(listenfd, 100);

	if (rt != 0) {
		ERRORLOG("listen error");
		exit(1);
	}

	rocket::FdEvent event(listenfd);
	event.listen(rocket::FdEvent::IN_EVENT, [listenfd]() {
		sockaddr_in peer_addr;
		socklen_t addr_len = sizeof(peer_addr);
		memset(&peer_addr, 0, sizeof(peer_addr));
		int clientfd = accept(listenfd, reinterpret_cast<sockaddr*>(&peer_addr),
		                      &addr_len);

		if (clientfd == -1) {
			ERRORLOG("accept error");
			exit(1);
		}
		char* ip = inet_ntoa(peer_addr.sin_addr);
		DEBUGLOG("accept client fd [%d],ip[%s : %d]", clientfd, ip,
		         ntohs(peer_addr.sin_port));
	});
	eventLoop->addEpollEvent(&event);

	int i = 0;
	rocket::TimerEvent::s_ptr timer_event = std::make_shared<rocket::TimerEvent>(
	    1000, true, [&i]() { INFOLOG("trigger timer event count = %d", i++); });
	eventLoop->addTimerEvent(timer_event);
	
	eventLoop->loop();
}