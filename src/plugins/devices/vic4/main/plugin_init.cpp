#include "mmemu_plugin_api.h"
#include "vic4.h"

static const SimPluginHostAPI* g_host = nullptr;

static IOHandler* createVIC4() {
    auto* vic = new VIC4();
    if (g_host && g_host->getLogger && g_host->logNamed) {
        vic->setLogger(g_host->getLogger("vic4"), g_host->logNamed);
    }
    return vic;
}

static DevicePluginInfo s_devices[] = {
    {"vic4", createVIC4}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "vic4",
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

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    return &s_manifest;
}
