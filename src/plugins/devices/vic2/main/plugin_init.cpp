#include "mmemu_plugin_api.h"
#include "vic2.h"

static const SimPluginHostAPI* g_host = nullptr;

static IOHandler* createVIC2() {
    auto* vic = new VIC2("VIC-II", 0xD000);
    if (g_host && g_host->getLogger && g_host->logNamed) {
        vic->setLogger(g_host->getLogger("vic2"), g_host->logNamed);
    }
    return vic;
}

static DevicePluginInfo s_devices[] = {
    {"6567", createVIC2}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "vic2",
    nullptr,        // displayName
    "1.0.0",
    nullptr,        // deps
    nullptr,        // supportedMachineIds
    0, nullptr,     // cores
    0, nullptr,     // toolchains
    1, s_devices,   // devices
    0, nullptr,     // machines
    0, nullptr,     // loaders
    0, nullptr      // cartridges
};

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    return &s_manifest;
}
