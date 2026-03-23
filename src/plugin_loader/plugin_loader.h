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
