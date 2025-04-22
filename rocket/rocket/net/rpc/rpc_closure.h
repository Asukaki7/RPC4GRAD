#ifndef ROCKET_NET_RPC_CLOSURE_H
#define ROCKET_NET_RPC_CLOSURE_H

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <functional>
namespace rocket {
class RpcClosure : public google::protobuf::Closure {
public:
    RpcClosure(std::function<void()> cb) : m_cb(cb) {}

    void Run() {
        m_cb();
    }
private:
    std::function<void()> m_cb{nullptr};
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RpcClosure);
};
} // namespace rocket
#endif