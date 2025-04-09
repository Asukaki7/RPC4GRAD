
#include "rocket/common/log.h"
#include "rocket/net/TCP/net_addr.h"
#include "rocket/common/config.h"




int main() {

	rocket::Config::setGlobalConfig("../conf/rocket.xml");

	rocket::Logger::InitGlobalLogger();

    rocket::IPNetAddr addr("127.0.0.1", 12345);
    DEBUGLOG("create addr %s", addr.toString().c_str());
    
}