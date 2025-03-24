#include "rocket/common/log.h"
#include <cstddef>
#include <pthread.h>

void* func(void* arg) {
	DEBUGLOG("test log in thread %s", "1");
    return NULL;
}

int main() {
	pthread_t thread;
	pthread_create(&thread, NULL, &func, NULL);
	DEBUGLOG("test log in main %s", "2");
	return 0;
}