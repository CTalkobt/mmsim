#include "plugin_tool_registry.h"
#include <iostream>

PluginToolRegistry& PluginToolRegistry::instance() {
    static PluginToolRegistry inst;
    return inst;
}

bool PluginToolRegistry::registerTool(const PluginMcpToolInfo* info) {
    if (!info || !info->toolName) return false;
    
    std::string name(info->toolName);
    if (m_tools.find(name) != m_tools.end()) {
        std::cerr << "Warning: Plugin tool '" << name << "' is already registered." << std::endl;
        return false;
    }
    
    m_tools[name] = *info;
    return true;
}

bool PluginToolRegistry::dispatch(const std::string& method, const std::string& paramsJson, std::string& resultJson) {
    auto it = m_tools.find(method);
    if (it == m_tools.end()) return false;
    
    char* result = nullptr;
    it->second.handle(paramsJson.c_str(), &result, it->second.ctx);
    
    if (result) {
        resultJson = result;
        if (it->second.freeString) {
            it->second.freeString(result);
        }
    } else {
        resultJson = "{\"error\": \"Plugin tool returned null result\"}";
    }
    
    return true;
}

void PluginToolRegistry::listTools(std::vector<std::string>& out) const {
    for (const auto& pair : m_tools) {
        out.push_back(pair.first);
    }
}

std::string PluginToolRegistry::getSchema(const std::string& method) const {
    auto it = m_tools.find(method);
    if (it != m_tools.end() && it->second.schemaJson) {
        return it->second.schemaJson;
    }
    return "{}";
}
