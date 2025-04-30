#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>
#include <string>
namespace rocket {
class Config {
public:
	Config(const char* xmlfile);

public:
	static Config* GetGlobalConfig();
	static void setGlobalConfig(const char* xmlfile);
	Config();

private:
	std::map<std::string, std::string> m_config_values;

public:
	std::string m_log_level;
	std::string m_log_file_name;
	std::string m_log_file_path;
	int m_log_max_file_size {0};
	int m_log_flush_interval {0}; // 日志同步间隔

	int m_port {0};
	int m_io_threads {0};
};

} // namespace rocket

#endif // ROCKET_COMMON_CONFIG_H