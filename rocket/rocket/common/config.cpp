#include "common/config.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
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
		if (xmlfile != nullptr) {
			g_config = new Config(xmlfile);
		} else {
			g_config = new Config();
		}
    }
}

Config::Config() {
	m_log_level = "DEBUG";
}


Config::Config(const char* xmlfile) {
	m_xml_document = new TiXmlDocument();

	bool rt = m_xml_document->LoadFile(xmlfile);

	if (!rt) {
		std::cout << "start rocket server error, failed to read config file"
		          << xmlfile << "error: " << m_xml_document->ErrorDesc() << std::endl;
		std::exit(0);
	}
	
	READ_XML_NODE(root, m_xml_document);
	READ_XML_NODE(log, root_node);
	READ_XML_NODE(server, root_node);

	READ_STR_FROM_XML_NODE(log_level, log_node);
	READ_STR_FROM_XML_NODE(log_file_name, log_node);
	READ_STR_FROM_XML_NODE(log_file_path, log_node);
	READ_STR_FROM_XML_NODE(log_max_file_size, log_node);
	READ_STR_FROM_XML_NODE(log_flush_interval, log_node);

	m_log_level = log_level_str;
	m_log_file_name = log_file_name_str;
	m_log_file_path = log_file_path_str;
	m_log_max_file_size = std::stoi(log_max_file_size_str);
	m_log_flush_interval = std::stoi(log_flush_interval_str);

	printf("LOG -- config:\n");
    printf("log_level: %s\n", m_log_level.c_str());
    printf("log_file_name: %s\n", m_log_file_name.c_str());
    printf("log_file_path: %s\n", m_log_file_path.c_str());
    printf("log_max_file_size: %d\n", m_log_max_file_size);
    printf("log_flush_interval: %d\n", m_log_flush_interval);

	READ_STR_FROM_XML_NODE(port, server_node);
	READ_STR_FROM_XML_NODE(io_threads, server_node);

	m_port = std::stoi(port_str);
	m_io_threads = std::stoi(io_threads_str);

	printf("server -- config: port: %d, io_threas: %d\n", m_port, m_io_threads);

}

Config::~Config() {
	if (m_xml_document) {
		delete m_xml_document;
		m_xml_document = nullptr;
	}
}

} // namespace rocket