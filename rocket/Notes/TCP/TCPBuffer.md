
## 为什么需要应用层buffer？
* 方便数据处理，特别是应用层的包组装和拆解
* 方便异步的发送（发送数据直接塞到缓冲区里面，等待epoll异步去发送）
* 提高发送效率，多个包合并一起发送

## Inbuffer
* 服务端调用read（系统socket底层read函数）成功从socket缓冲区读到数据，会写入到Inbuffer后面
* 服务端从Inbuffer前面读取到数据，进行解码得到请求

## OutBuffer 
* 服务端向外发送数据，会将数据编码后写入到OutBuffer后面
* 服务端在fd可写的情况下，调用write将OutBuffer里面的数据全部发送出去