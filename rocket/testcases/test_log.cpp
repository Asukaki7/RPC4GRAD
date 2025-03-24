#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include <cstddef>
#include <pthread.h>

void* func(void* arg) {
	int i = 100;
	while (i--) {
		DEBUGLOG("test Debug log in thread %s", "2");
		INFOLOG("test Info log in thread %s", "3");
	}

	return NULL;
}

int main() {

	rocket::Config::setGlobalConfig("../conf/rocket.xml");

	rocket::Logger::InitGlobalLogger();

	pthread_t thread;
	pthread_create(&thread, NULL, &func, NULL);

	int i = 100;
	while (i--) {
		DEBUGLOG("test Debug log in main %s", "2");
		INFOLOG("test Info log in main %s", "3");
	}
	

	pthread_join(thread, nullptr);
	return 0;
}