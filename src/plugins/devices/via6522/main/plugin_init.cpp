#include "mmemu_plugin_api.h"
#include "via6522.h"

static IOHandler* createVIA6522() {
    // Default values, should be configured by the machine
    return new VIA6522("6522", 0);
}

static DevicePluginInfo s_devices[] = {
    {"6522", createVIA6522}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "via6522",
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
