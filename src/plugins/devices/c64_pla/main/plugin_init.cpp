#include "mmemu_plugin_api.h"
#include "c64_pla.h"

static IOHandler* createC64PLA() {
    return new C64PLA();
}

static DevicePluginInfo s_devices[] = {
    {"c64pla", createC64PLA}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-c64-pla",
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
