#ifndef ROCKET_COMMON_RUN_TIME_H
#define ROCKET_COMMON_RUN_TIME_H

#include <string>



namespace rocket {

class RpcInterface;

class RunTime {

public:
	static RunTime& getRuntime() {
		static RunTime instance;
		return instance;
	}

	std::string getMsgId() const;
    std::string getMethodName() const;
	void setMsgId(std::string);
	void setMethodName(std::string);
	void setRpcInterface(RpcInterface*);
	RpcInterface* getRpcInterface() const;
private:
	std::string m_msgId;
	std::string m_method_name;
	RpcInterface* m_rpc_interface {nullptr};
private:
	RunTime() = default;
	RunTime(const RunTime& rhs) = delete;
	RunTime& operator=(const RunTime& rhs) = delete;
};

} // namespace rocket

#endif // ROCKET_COMMON_RUN_TIME_H