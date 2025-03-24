#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>
#include <string>
namespace rocket {
class Config {
public:
	Config(const char* xmlfile);
    
private:
	Config() = default;
    std::map<std::string, std::string> m_config_values;
};



} // namespace rocket

#endif // ROCKET_COMMON_CONFIG_H