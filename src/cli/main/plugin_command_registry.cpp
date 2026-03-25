#include "plugin_command_registry.h"
#include <iostream>

PluginCommandRegistry& PluginCommandRegistry::instance() {
    static PluginCommandRegistry inst;
    return inst;
}

bool PluginCommandRegistry::registerCommand(const PluginCommandInfo* info) {
    if (!info || !info->name) return false;
    
    std::string name(info->name);

    for (const auto& b : m_builtIns) {
        if (name == b) {
            std::cerr << "Error: Plugin command '" << name << "' collides with built-in command." << std::endl;
            return false;
        }
    }

    if (m_commands.find(name) != m_commands.end()) {
        std::cerr << "Warning: Plugin command '" << name << "' is already registered." << std::endl;
        return false;
    }
    
    m_commands[name] = *info;
    return true;
}

bool PluginCommandRegistry::dispatch(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return false;
    
    auto it = m_commands.find(tokens[0]);
    if (it == m_commands.end()) return false;
    
    std::vector<const char*> argv;
    for (const auto& t : tokens) {
        argv.push_back(t.c_str());
    }
    
    int result = it->second.execute((int)argv.size(), argv.data(), it->second.ctx);
    (void)result;
    return true; // We handled it, regardless of return code.
}

void PluginCommandRegistry::listCommands(std::vector<std::string>& out) const {
    for (const auto& pair : m_commands) {
        std::string s = "  " + pair.first;
        if (pair.second.usage) {
            s += " - " + std::string(pair.second.usage);
        }
        out.push_back(s);
    }
}

void PluginCommandRegistry::registerBuiltIn(const std::string& name) {
    m_builtIns.push_back(name);
}
