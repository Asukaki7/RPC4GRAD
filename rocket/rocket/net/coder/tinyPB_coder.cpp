#include "rocket/net/coder/tinyPB_coder.h"
#include "log.h"
#include "rocket/common/util.h"
#include "rocket/net/coder/tinyPB_protocol.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <vector>

namespace rocket {

void TinyPBCoder::encode(std::vector<AbstractProtocol::s_ptr>& message,
                         TcpBuffer::s_ptr out_buffer) {
	for (auto& i : message) {
		std::shared_ptr<TinyPBProtocol> msg =
		    std::dynamic_pointer_cast<TinyPBProtocol>(i);
		int len = 0;
		const char* buffer = encodeTinyPB(msg, len);
		if (buffer != nullptr && len != 0) {
			out_buffer->writeToBuffer(buffer, len);
			free(const_cast<char*>(buffer));
			buffer = nullptr;
			len = 0;
		}
	}
}

void TinyPBCoder::decode(std::vector<AbstractProtocol::s_ptr>& out_message,
                         TcpBuffer::s_ptr buffer) {

	while (1) {
		// 1. 遍历buffer 找到pb_start 找到之后 解析出整包的长度
		// 然后得到结束符的位置
		// 判断是否为pd_end

		std::vector<char> tmp = buffer->m_buffer;

		int start_index = buffer->getReadIndex();

		int end_index{-1};

		int pk_len{0};

		bool parse_success{false};
		int i{0};
		for (i = start_index; i < buffer->getWriteIndex(); i++) {
			if (tmp[i] == TinyPBProtocol::PB_START) {
				// 从下找四个字节 由于是网络字节序（大端）需要转为主机字节序
				if (i + 1 < buffer->getWriteIndex()) {
					pk_len = getInt32FromNetByte(&tmp[i + 1]);
					DEBUGLOG("get pk_len = %d", pk_len);

					int j = i + pk_len - 1;

					if (j >= buffer->getWriteIndex()) {
						continue;
					}
					if (tmp[j] == TinyPBProtocol::PB_END) {
						// 解析成功
						start_index = i;
						end_index = j;
						parse_success = true;
						break;
					}
				}
			}
		}
		if (i >= buffer->getWriteIndex()) {
			DEBUGLOG("decode end, read all buffer data");
			return;
		}

		if (parse_success) {
			buffer->moveReadIndex(pk_len);
			TinyPBProtocol::s_ptr message = std::make_shared<TinyPBProtocol>();

			message->setPkLength(pk_len);

			int req_id_len_index = start_index +
			                       sizeof(TinyPBProtocol::PB_START) +
			                       sizeof(message->getPkLength());
			if (req_id_len_index >= end_index) {
				message->setParaseSuccess(false);
				ERRORLOG("parse error, req_id_len_index [%d] >= end_index [%d]",
				         req_id_len_index, end_index);
				continue;
			}
			message->setReqIdLen(getInt32FromNetByte(&tmp[req_id_len_index]));
			DEBUGLOG("parse req_id_len = %d", message->getReqIdLen());

			int req_id_index =
			    req_id_len_index + sizeof(message->getReqIdLen());
			char req_id[100] = {0};
			memcpy(&req_id[0], &tmp[req_id_index], message->getReqIdLen());
			message->setReqId(std::string(req_id, message->getReqIdLen()));
			DEBUGLOG("parse req_id = %s", message->getReqId().c_str());

			int method_name_len_index = req_id_index + message->getReqIdLen();
			if (method_name_len_index >= end_index) {
				message->setParaseSuccess(false);
				ERRORLOG(
				    "parse error, method_name_len_index [%d] >= end_index [%d]",
				    method_name_len_index, end_index);
				continue;
			}

			message->setMethodNameLen(
			    getInt32FromNetByte(&tmp[method_name_len_index]));

			int method_name_index =
			    method_name_len_index + sizeof(message->getMethodNameLen());
			char method_name[512] = {0};
			memcpy(&method_name[0], &tmp[method_name_index],
			            message->getMethodNameLen());
			message->setMethodName(
			    std::string(method_name, message->getMethodNameLen()));
			DEBUGLOG("parse method_name = %s",
			         message->getMethodName().c_str());

			int err_code_index =
			    method_name_index + message->getMethodNameLen();
			if (err_code_index >= end_index) {
				message->setParaseSuccess(false);
				ERRORLOG("parse error, err_code_index [%d] >= end_index [%d]",
				         err_code_index, end_index);
				continue;
			}
			message->setErrCode(getInt32FromNetByte(&tmp[err_code_index]));

			int error_info_len_index =
			    err_code_index + sizeof(message->getErrCode());
			if (error_info_len_index >= end_index) {
				message->setParaseSuccess(false);
				ERRORLOG(
				    "parse error, error_info_len_index [%d] >= end_index [%d]",
				    error_info_len_index, end_index);
				continue;
			}

			message->setErrInfoLen(
			    getInt32FromNetByte(&tmp[error_info_len_index]));

			int error_info_index =
			    error_info_len_index + sizeof(message->getErrInfoLen());
			char error_info[512] = {0};
			memcpy(&error_info[0], &tmp[error_info_index],
			            message->getErrInfoLen());
			message->setErrInfo(
			    std::string(error_info, message->getErrInfoLen()));
			DEBUGLOG("parse error_info = %s", message->getErrInfo().c_str());

			int pb_body_len = message->getPkLength() - message->getReqIdLen() -
			                  message->getMethodNameLen() -
			                  -message->getErrInfoLen() - 2 - 24;

			int pb_body_index = error_info_index + message->getErrInfoLen();
			message->setPbBody(std::string(&tmp[pb_body_index], pb_body_len));
			DEBUGLOG("parse pb_body = %s", message->getPbBody().c_str());
			
			// TODO: 计算校验和

			message->setParaseSuccess(true);
			out_message.push_back(message);
		}
	}
}

const char* TinyPBCoder::encodeTinyPB(TinyPBProtocol::s_ptr message,
                                      int32_t& len) {
	if (message->getReqId().empty()) {
		message->setReqId("114514");
	}
	DEBUGLOG("req_id = %s", message->getReqId().c_str());
	int pk_len = 2 + 24 + message->getReqId().length() + message->getMethodName().length() +
	             message->getErrInfo().length() + message->getPbBody().size();
	DEBUGLOG("pk_len = %d", pk_len);

	char* buffer = reinterpret_cast<char*>(malloc(pk_len));
	char* tmp = buffer;

	*tmp = TinyPBProtocol::PB_START;
	tmp++;

	int32_t pk_len_net = htonl(pk_len);
	memcpy(tmp, &pk_len_net, sizeof(pk_len_net));
	tmp += sizeof(pk_len_net);

	int32_t req_id_len = message->getReqId().size();
	int32_t req_id_len_net = htonl(req_id_len);
	memcpy(tmp, &req_id_len_net, sizeof(req_id_len_net));
	tmp += sizeof(req_id_len_net);

	if (!message->getReqId().empty()) {
		memcpy(tmp, message->getReqId().c_str(), req_id_len);
		tmp += req_id_len;
	}

	int method_name_len = message->getMethodName().size();
	int32_t method_name_len_net = htonl(method_name_len);
	memcpy(tmp, &method_name_len_net, sizeof(method_name_len_net));
	tmp += sizeof(method_name_len_net);

	if (!message->getMethodName().empty()) {
		memcpy(tmp, message->getMethodName().c_str(),
		       	message->getMethodName().size());
		tmp += message->getMethodName().size();
	}

	int32_t err_code_net = htonl(message->getErrCode());
	memcpy(tmp, &err_code_net, sizeof(err_code_net));
	tmp += sizeof(err_code_net);

	int err_info_len = message->getErrInfoLen();
	int32_t err_info_len_net = htonl(err_info_len);
	memcpy(tmp, &err_info_len_net, sizeof(err_info_len_net));
	tmp += sizeof(err_info_len_net);

	if (!message->getErrInfo().empty()) {
		memcpy(tmp, message->getErrInfo().c_str(), message->getErrInfo().size());
		tmp += message->getErrInfo().size();
	}

	if (!message->getPbBody().empty()) {
		memcpy(tmp, message->getPbBody().c_str(), message->getPbBody().size());
		tmp += message->getPbBody().size();
	}

	int32_t check_sum_net = htonl(1);
	memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
	tmp += sizeof(check_sum_net);

	*tmp = TinyPBProtocol::PB_END;

	message->setPkLength(pk_len);
	message->setReqIdLen(req_id_len);
	message->setMethodNameLen(method_name_len);
	message->setErrInfoLen(err_info_len);
	message->setParaseSuccess(true);
	len = pk_len;

	DEBUGLOG("encode message [%s] success", message->getReqId().c_str());

	return buffer;
}

} // namespace rocket