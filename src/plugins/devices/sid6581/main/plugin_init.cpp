#include "mmemu_plugin_api.h"
#include "sid6581.h"

static IOHandler* createSID6581() {
    return new SID6581("SID", 0xD400);
}

static DevicePluginInfo s_devices[] = {
    {"6581", createSID6581}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "sid6581",
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

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    (void)host;
    return &s_manifest;
}
