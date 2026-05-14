#include "mmemu_plugin_api.h"
#include "sid_pair.h"

static IOHandler* createSidPair() {
    return new SidPair();
}

static DevicePluginInfo s_devices[] = {
    {"sid_pair", createSidPair}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "sid_pair",
    "MEGA65 Dual SID",
    "1.0.0",
    nullptr, nullptr,
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
