#include "rocket/common/log.h"
#include "rocket/net/rpc/rpc_closure.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_interface.h"

namespace rocket {

RpcInterface::RpcInterface(RpcClosure* done, RpcController* controller)
    : m_done(done), m_controller(controller) {}


void RpcInterface::reply() {
    if (m_done) {
        m_done->Run();
    }
}


}
