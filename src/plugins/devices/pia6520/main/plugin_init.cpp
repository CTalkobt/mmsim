#include "mmemu_plugin_api.h"
#include "pia6520.h"

static IOHandler* createPIA6520() {
    return new PIA6520("6520", 0);
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
    0, nullptr
};

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->deviceRegistry) DeviceRegistry::setInstance(host->deviceRegistry);
    return &s_manifest;
}
