# 为什么要自定义协议格式

既然用了Prootobuf做序列化，为什么不直接把序列化后的结果直接发送，而要在上面在自定义一些字段？
- **为了方便分割请求**：因为protobuf后的结果是一串无意义的字节流，你无法区分哪里是开始或者结束。比如说把两个Message对象序列化后的结果排在一起，你甚至无法分开这两个请求。在TCP传输是按照字节流传输，没有包的概念，因此应用层就更无法去区分了。
- **为了定位**：加上MsgID等信息，能帮助我们匹配一次RPC的请求和响应，不会串包
- **错误提升**：加上错误信息，能很容易知道RPC失败的原因，方便问题定位


TODO
增加一个image，展示一下自定义协议的格式
# 自定义协议格式
```c++
/*
**  min of package is: 1 + 4 + 4 + 4 + 4 + 4 + 4 + 1 = 26 bytes
*/
char start;                         // 代表报文的开始， 一般是 0x02
int32_t pk_len {0};                 // 整个包长度，单位 byte
int32_t msg_req_len {0};            // msg_req 字符串长度
std::string msg_req;                // msg_req,标识一个 rpc 请求或响应。 一般来说 请求 和 响应使用同一个 msg_req.
int32_t service_name_len {0};       // service_name 长度
std::string service_full_name;      // 完整的 rpc 方法名， 如 QueryService.query_name
int32_t err_code {0};               // 框架级错误代码. 0 代表调用正常，非 0 代表调用失败
int32_t err_info_len {0};           // err_info 长度
std::string err_info;               // 详细错误信息， err_code 非0时会设置该字段值
std::string pb_data;                // 业务 protobuf 数据，由 google 的 protobuf 序列化后得到
int32_t check_num {0};             // 包检验和，用于检验包数据是否有损坏
char end;                           // 代表报文结束，一般是 0x03
```
以下是协议各段内容的连续表格说明：

| **字段名**            | **数据类型**      | **长度（字节）** | **描述**                                                                 |
|-----------------------|-------------------|------------------|-------------------------------------------------------------------------|
| `start`               | `char`            | 1                | 报文起始标志，固定为 `0x02`。                                           |
| `pk_len`              | `int32_t`         | 4                | 整个报文的长度（包含所有字段），单位字节,包含开始符和结束符。                               |
| `msg_req_len`         | `int32_t`         | 4                | `msg_req` 字段的字符串长度（不含终止符）。                              |
| `msg_req`             | `std::string`     | 变长             | 唯一标识 RPC 请求/响应的字符串，请求和响应需保持一致。                   |
| `service_name_len`    | `int32_t`         | 4                | `service_full_name` 字段的字符串长度。                                  |
| `service_full_name`   | `std::string`     | 变长             | 完整的 RPC 方法名，格式为 `<服务名>.<方法名>`，例如 `QueryService.query`。 |
| `err_code`            | `int32_t`         | 4                | RPC调用发生系统异常，此错误码被设置。框架级错误码：<br> `0` 表示成功，非 `0` 表示失败。                       |
| `err_info_len`        | `int32_t`         | 4                | `err_info` 字段的字符串长度（仅在 `err_code ≠ 0` 时有效）。              |
| `err_info`            | `std::string`     | 变长             | 详细错误信息，仅在 `err_code ≠ 0` 时填充。                              |
| `pb_data`             | `std::string`     | 变长             | 业务数据，由 Protobuf 序列化后的二进制内容。                             |
| `check_num`           | `int32_t`         | 4                | 校验和，用于验证报文完整性。                        |
| `end`                 | `char`            | 1                | 报文结束标志，固定为 `0x03`。                                           |



**注意: 所有的整数都是网络字节序 大端存储**
