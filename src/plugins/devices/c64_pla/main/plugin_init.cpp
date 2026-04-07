#include "mmemu_plugin_api.h"
#include "c64_pla.h"

static IOHandler* createC64PLA() {
    return new C64PLA();
}

static DevicePluginInfo s_devices[] = {
    {"c64_pla", createC64PLA}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "c64-pla",
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
    (void)host;
    return &s_manifest;
}
