#ifndef ROCKET_NET_CODER_TINYPB_PROTOCOL_H
#define ROCKET_NET_CODER_TINYPB_PROTOCOL_H

#include "rocket/net/coder/abstract_protocol.h"

namespace rocket {

class TinyPBProtocol : public AbstractProtocol {
public:
	TinyPBProtocol() = default;
	~TinyPBProtocol() = default;
    typedef std::shared_ptr<TinyPBProtocol> s_ptr;
	constexpr static char PB_START = 0x02;
	constexpr static char PB_END = 0x03;

public:
	int32_t getPkLength() const { return m_pk_length; }
	void setPkLength(int32_t pk_length) { m_pk_length = pk_length; }

	int32_t getReqIdLen() const { return m_req_id_len; }
	void setReqIdLen(int32_t req_id_len) { m_req_id_len = req_id_len; }

	int32_t getMethodNameLen() const { return m_method_name_len; }
	void setMethodNameLen(int32_t method_name_len) {
		m_method_name_len = method_name_len;
	}

	const std::string& getMethodName() const { return m_method_name; }
	void setMethodName(const std::string& method_name) {
		m_method_name = method_name;
		m_method_name_len = method_name.length();
	}

	int32_t getErrCode() const { return m_err_code; }
	void setErrCode(int32_t err_code) { m_err_code = err_code; }

	int32_t getErrInfoLen() const { return m_err_info_len; }
	void setErrInfoLen(int32_t err_info_len) { m_err_info_len = err_info_len; }

	const std::string& getErrInfo() const { return m_err_info; }
	void setErrInfo(const std::string& err_info) {
		m_err_info = err_info;
		m_err_info_len = err_info.length();
	}

	std::string& getPbBody() { return m_pb_body; }
	void setPbBody(const std::string& pb_body) { m_pb_body = pb_body; }


	int32_t getCheckSum() const { return m_check_sum; }
	void setCheckSum(int32_t check_sum) { m_check_sum = check_sum; }

    bool getParaseSuccess() const { return m_parase_success; }
	void setParaseSuccess(bool parase_success) { m_parase_success = parase_success; }

private:
	//*  头部长度
	int32_t m_pk_length{0};

	//* 请求字段长度
	int32_t m_req_id_len{0};
	// req_id 继承自AbstractCoder

	//* 方法名长度及方法名
	int32_t m_method_name_len{0};
	std::string m_method_name;

	//* 错误码
	int32_t m_err_code{0};

	//* 错误信息长度及错误信息
	int32_t m_err_info_len{0};
	std::string m_err_info;

	//* 数据体
	std::string m_pb_body;

	//* 校验和
	int32_t m_check_sum{0};

    bool m_parase_success{false};
};

} // namespace rocket

#endif