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
	Config() = default;

private:
	std::map<std::string, std::string> m_config_values;

public:
	std::string m_log_level;
};

} // namespace rocket

#endif // ROCKET_COMMON_CONFIG_H