#include "common/config.h"
#include <cstdlib>

#include <tinyxml/tinyxml.h>
#include <iostream>

#define READ_XML_NODE(name, root) \
    TiXmlElement* name##_node = root->FirstChildElement(#name); \
    if (!name##_node) { \
        std::cout << "start rocket server error, failed to read " << #name << " node" << std::endl; \
        std::exit(0); \
    }


namespace rocket {
Config::Config(const char* xmlfile) {
    TiXmlDocument* xml_document = new TiXmlDocument();

    bool rt = xml_document->LoadFile(xmlfile);

    if (!rt) {
        std::cout << "start rocket server error, failed to read config file" << xmlfile << std::endl;
        std::exit(0);
    }

    TiXmlElement* root_node = xml_document->FirstChildElement("root");
    if (!root_node) {
        std::cout << "start rocket server error, failed to read root node" << std::endl;
        std::exit(0);
    }

    TiXmlElement* log_node = root_node->FirstChildElement("log");
    if (!log_node) {
        std::cout << "start rocket server error, failed to read log node" << std::endl;
        std::exit(0);
    }

}
}