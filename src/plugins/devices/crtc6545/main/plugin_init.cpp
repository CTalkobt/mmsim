#include "include/mmemu_plugin_api.h"
#include "crtc6545.h"
#include "libdevices/main/device_registry.h"

static IOHandler* createCRTC6545() {
    return new CRTC6545();
}

static DevicePluginInfo s_devices[] = {
    { "6545", createCRTC6545 }
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "crtc6545",
    "MOS 6545 CRTC",
    "1.0.0",
    nullptr, nullptr, // deps, supportedMachineIds
    0, nullptr,       // cores
    0, nullptr,       // toolchains
    1, s_devices,
    0, nullptr        // machines
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->deviceRegistry) {
        DeviceRegistry::setInstance(host->deviceRegistry);
    }
    return &s_manifest;
}
