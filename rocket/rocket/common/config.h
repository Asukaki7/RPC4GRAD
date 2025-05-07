#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>
#include <string>
#include <tinyxml/tinyxml.h>
#include "rocket/net/TCP/net_addr.h"

namespace rocket {

struct Rpcstub {
	std::string name;
	NetAddr::s_ptr addr;
	int timeout {2000};
};

class Config {
public:
	Config(const char* xmlfile);
	Config();
	~Config();

public:
	static Config* GetGlobalConfig();
	static void setGlobalConfig(const char* xmlfile);

private:
	std::map<std::string, std::string> m_config_values;

public:
	std::string m_log_level;
	std::string m_log_file_name;
	std::string m_log_file_path;
	int m_log_max_file_size{0};
	int m_log_flush_interval{0}; // 日志同步间隔

	int m_port{0};
	int m_io_threads{0};

	TiXmlDocument* m_xml_document{nullptr};

	std::map<std::string, Rpcstub> m_rpc_stubs;
};

} // namespace rocket

#endif // ROCKET_COMMON_CONFIG_H