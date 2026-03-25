#include "mmemu_plugin_api.h"
#include "vic6560.h"

static IOHandler* createVIC6560() {
    return new VIC6560("VIC-I", 0x9000);
}

static DevicePluginInfo s_devices[] = {
    {"6560", createVIC6560}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-vic6560",
    "1.0.0",
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
