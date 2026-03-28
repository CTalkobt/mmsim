#include "mmemu_plugin_api.h"
#include "cia6526.h"

static IOHandler* createCIA6526() {
    return new CIA6526("CIA", 0);
}

static DevicePluginInfo s_devices[] = {
    {"6526", createCIA6526}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-cia6526",
    nullptr,        // displayName
    "1.0.0",
    nullptr,        // deps
    nullptr,        // supportedMachineIds
    0, nullptr,
    0, nullptr,
    1, s_devices,
    0, nullptr
};

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->deviceRegistry) DeviceRegistry::setInstance(host->deviceRegistry);
    return &s_manifest;
}
