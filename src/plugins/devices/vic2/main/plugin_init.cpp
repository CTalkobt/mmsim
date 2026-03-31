#include "mmemu_plugin_api.h"
#include "vic2.h"

static IOHandler* createVIC2() {
    return new VIC2("VIC-II", 0xD000);
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
    0, nullptr,
    0, nullptr,
    1, s_devices,
    0, nullptr
};

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    (void)host;
    return &s_manifest;
}
