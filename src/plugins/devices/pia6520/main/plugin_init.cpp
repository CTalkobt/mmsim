#include "mmemu_plugin_api.h"
#include "pia6520.h"
#include "libdevices/main/device_registry.h"

static const SimPluginHostAPI* g_host = nullptr;

static IOHandler* createPIA6520() {
    auto* pia = new PIA6520("6520", 0);
    if (g_host && g_host->getLogger && g_host->logNamed) {
        pia->setLogger(g_host->getLogger("6520"), g_host->logNamed);
    }
    return pia;
}

static DevicePluginInfo s_devices[] = {
    {"6520", createPIA6520}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "pia6520",
    nullptr,        // displayName
    "1.0.0",
    nullptr,        // deps
    nullptr,        // supportedMachineIds
    0, nullptr,
    0, nullptr,
    1, s_devices,
    0, nullptr,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    return &s_manifest;
}
