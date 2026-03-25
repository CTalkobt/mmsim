#pragma once

#include <string>
#include <vector>
#include "include/mmemu_plugin_api.h"

class PluginLoader {
public:
    static PluginLoader& instance();

    bool load(const std::string& path);
    void loadFromDir(const std::string& dir);
    
    void unloadAll();

    // Host-provided extension registration stubs
    void setPaneRegisterFn(void (*fn)(const PluginPaneInfo*));
    void setCommandRegisterFn(void (*fn)(const PluginCommandInfo*));
    void setMcpToolRegisterFn(void (*fn)(const PluginMcpToolInfo*));

private:
    PluginLoader() = default;
    
    struct LoadedPlugin {
        std::string path;
        void* handle;
        SimPluginManifest* manifest;
    };

    std::vector<LoadedPlugin> m_plugins;

    void registerPluginItems(SimPluginManifest* manifest);
};
