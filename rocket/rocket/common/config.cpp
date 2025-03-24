#include "common/config.h"
#include <cstdlib>
#include <iostream>
#include <tinyxml/tinyxml.h>

#define READ_XML_NODE(name, root)                                          \
	TiXmlElement* name##_node = root->FirstChildElement(#name);            \
	if (!name##_node) {                                                    \
		std::cout << "start rocket server error, failed to read " << #name \
		          << " node" << std::endl;                                 \
		std::exit(0);                                                      \
	}

#define READ_STR_FROM_XML_NODE(name, parent)                               \
	TiXmlElement* name##_node = parent->FirstChildElement(#name);          \
	if (!name##_node || !name##_node->GetText()) {                         \
		std::cout << "start rocket server error, failed to read " << #name \
		          << " node" << std::endl;                                 \
		std::exit(0);                                                      \
	}                                                                      \
	std::string name##_str = std::string(name##_node->GetText());

namespace rocket {


static Config* g_config = nullptr;
Config* Config::GetGlobalConfig() {
    return g_config;
}

void Config::setGlobalConfig(const char* xmlfile) {
    if (g_config == nullptr) {
        g_config = new Config(xmlfile);
    }
}



Config::Config(const char* xmlfile) {
	TiXmlDocument* xml_document = new TiXmlDocument();

	bool rt = xml_document->LoadFile(xmlfile);

	if (!rt) {
		std::cout << "start rocket server error, failed to read config file"
		          << xmlfile << std::endl;
		std::exit(0);
	}

	READ_XML_NODE(root, xml_document);
	READ_XML_NODE(log, root_node);

	READ_STR_FROM_XML_NODE(log_level, log_node);


    m_log_level = log_level_str;
}
} // namespace rocket