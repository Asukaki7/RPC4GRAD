#ifndef ROCKET_RPC_RPC_INTERFACE_H
#define ROCKET_RPC_RPC_INTERFACE_H

#include "rocket/net/rpc/rpc_closure.h"
#include "rocket/net/rpc/rpc_controller.h"

namespace rocket {

/*
* Rpc Interface base Class 
* All interface class should inherit from this class
*/
class RpcInterface {
public:
    RpcInterface(RpcClosure* done, RpcController* controller);
    virtual ~RpcInterface() = default;

    // core business deal method
    virtual void run() = 0;

    // set error code and error info to response message
    virtual void set_error(long long err_code, const std::string& err_info) = 0;

    // reply to client
    // you should call is when you want to response back
    // it means this rpc method is done
    virtual void reply() = 0;


protected:
    RpcClosure* m_done {nullptr}; //callback
    RpcController* m_controller {nullptr};

};




}





#endif