#ifndef ROCKET_NET_ABSTRACT_CODER_H
#define ROCKET_NET_ABSTRACT_CODER_H

#include "rocket/net/TCP/tcp_buffer.h"
#include "rocket/net/coder/abstract_protocol.h"
#include <vector>


namespace rocket {

class AbstractCoder{

public:
    typedef std::shared_ptr<AbstractCoder> s_ptr;

    // 编码 将messsage对象转换为字节流 写入到buffer
    virtual void encode(std::vector<AbstractProtocol::s_ptr>& message, TcpBuffer::s_ptr out_buffer) = 0;

    // 解码 将buffer中的字节流转换为message对象 
    virtual void decode(std::vector<AbstractProtocol::s_ptr>& out_message, TcpBuffer::s_ptr buffer) = 0;

    virtual ~AbstractCoder() {}
};


}





#endif