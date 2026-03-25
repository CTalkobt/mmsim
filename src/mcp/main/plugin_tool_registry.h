#pragma once

#include "mmemu_plugin_api.h"
#include <string>
#include <vector>
#include <map>

/**
 * Singleton registry for plugin-provided MCP tools.
 */
class PluginToolRegistry {
public:
    static PluginToolRegistry& instance();

    /**
     * Registers a new tool. Rejects duplicates.
     */
    bool registerTool(const PluginMcpToolInfo* info);

    /**
     * Dispatches a method if found.
     * @param method JSON-RPC method name.
     * @param paramsJson JSON-formatted parameters.
     * @param resultJson JSON-formatted result (out).
     * @return true if handled, false if not found.
     */
    bool dispatch(const std::string& method, const std::string& paramsJson, std::string& resultJson);

    /**
     * Lists all registered tool names.
     */
    void listTools(std::vector<std::string>& out) const;

    /**
     * Retrieves the schema for a tool.
     */
    std::string getSchema(const std::string& method) const;

private:
    PluginToolRegistry() = default;
    std::map<std::string, PluginMcpToolInfo> m_tools;
};
