#pragma once

#include "mmemu_plugin_api.h"
#include <string>
#include <vector>
#include <map>

/**
 * Singleton registry for plugin-provided CLI commands.
 */
class PluginCommandRegistry {
public:
    static PluginCommandRegistry& instance();

    /**
     * Registers a new command. Rejects duplicates.
     * @return true if registered, false if it collides with an existing command.
     */
    bool registerCommand(const PluginCommandInfo* info);

    /**
     * Dispatches a command if found.
     * @param tokens Command and arguments.
     * @return true if handled, false if not found.
     */
    bool dispatch(const std::vector<std::string>& tokens);

    /**
     * Lists all registered commands for help output.
     */
    void listCommands(std::vector<std::string>& out) const;

    /**
     * Registers a built-in command name to prevent plugins from overriding it.
     */
    void registerBuiltIn(const std::string& name);

private:
    PluginCommandRegistry() = default;
    std::map<std::string, PluginCommandInfo> m_commands;
    std::vector<std::string> m_builtIns;
};
