#include "rocket/net/wakeup_fd_event.h"
#include "fd_event.h"
#include "log.h"
#include <cerrno>
#include <unistd.h>



namespace rocket {
WakeupFdEvent::WakeupFdEvent(int fd) : FdEvent(fd) {

}

WakeupFdEvent::~WakeupFdEvent() {
    
}



void WakeupFdEvent::wakeup() {
    char buf[8] = {'a'};

    int rt = write(m_fd, buf, 8);

    if (rt != 8) {
        ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd);
    }
    DEBUGLOG("success read 8 bytes");
}


}