#include "plugin_loader.h"
#include <dlfcn.h>
#include <iostream>
#include <filesystem>

#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include "libdevices/main/device_registry.h"

namespace fs = std::filesystem;

PluginLoader& PluginLoader::instance() {
    static PluginLoader s_instance;
    return s_instance;
}

static void hostLog(int level, const char* msg) {
    std::cerr << "[Plugin Log " << level << "] " << msg << std::endl;
}

static SimPluginHostAPI s_hostAPI = {
    hostLog,
    &CoreRegistry::instance(),
    &MachineRegistry::instance(),
    &DeviceRegistry::instance(),
    &ToolchainRegistry::instance()
};

bool PluginLoader::load(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
        return false;
    }

    MmemuPluginInitFn initFn = (MmemuPluginInitFn)dlsym(handle, "mmemuPluginInit");
    if (!initFn) {
        std::cerr << "Plugin missing mmemuPluginInit: " << path << std::endl;
        dlclose(handle);
        return false;
    }

    SimPluginManifest* manifest = initFn(&s_hostAPI);
    if (!manifest) {
        std::cerr << "Plugin initialization failed: " << path << std::endl;
        dlclose(handle);
        return false;
    }

    if (MMEMU_PLUGIN_API_MAJOR(manifest->apiVersion) != MMEMU_PLUGIN_API_MAJOR(MMEMU_PLUGIN_API_VERSION)) {
        std::cerr << "Plugin API major version mismatch: plugin="
                  << MMEMU_PLUGIN_API_MAJOR(manifest->apiVersion)
                  << " host=" << MMEMU_PLUGIN_API_MAJOR(MMEMU_PLUGIN_API_VERSION) << std::endl;
        dlclose(handle);
        return false;
    }

    m_plugins.push_back({path, handle, manifest});
    registerPluginItems(manifest);

    std::cerr << "Loaded plugin: " << manifest->pluginId << " v" << manifest->version << std::endl;
    return true;
}

void PluginLoader::loadFromDir(const std::string& dir) {
    if (!fs::exists(dir)) return;
    std::vector<std::string> deferred;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".so") {
            if (!load(entry.path().string()))
                deferred.push_back(entry.path().string());
        }
    }
    // Retry plugins that failed on first pass — their dependencies may now be loaded.
    for (const auto& path : deferred) {
        load(path);
    }
}

void PluginLoader::unloadAll() {
    for (auto& p : m_plugins) {
        dlclose(p.handle);
    }
    m_plugins.clear();
}

void PluginLoader::registerPluginItems(SimPluginManifest* manifest) {
    // Cores
    for (int i = 0; i < manifest->coreCount; ++i) {
        auto& c = manifest->cores[i];
        CoreRegistry::instance().registerCore(c.name, c.variant, c.licenseClass, c.create);
    }

    // Toolchains
    for (int i = 0; i < manifest->toolchainCount; ++i) {
        auto& t = manifest->toolchains[i];
        ToolchainRegistry::instance().registerToolchain(t.isa, t.createDisassembler, t.createAssembler);
    }

    // Machines
    for (int i = 0; i < manifest->machineCount; ++i) {
        auto& m = manifest->machines[i];
        MachineRegistry::instance().registerMachine(m.machineId, m.create);
    }

    // Devices
    for (int i = 0; i < manifest->deviceCount; ++i) {
        auto& d = manifest->devices[i];
        DeviceRegistry::instance().registerDevice(d.name, d.create);
    }
}
