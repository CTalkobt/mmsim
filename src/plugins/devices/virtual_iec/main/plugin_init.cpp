#include "mmemu_plugin_api.h"
#include "virtual_iec.h"

static IOHandler* createVirtualIEC() {
    return new VirtualIECBus(8);
}

static DevicePluginInfo s_devices[] = {
    {"virtual_iec", createVirtualIEC}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "virtual_iec",
    "Virtual IEC Device",
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
    (void)host;
    return &s_manifest;
}
