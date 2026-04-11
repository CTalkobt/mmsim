#include "mmemu_plugin_api.h"
#include "virtual_iec.h"

static const SimPluginHostAPI* g_host = nullptr;

static IOHandler* createVirtualIEC() {
    auto* iec = new VirtualIECBus(8);
    if (g_host && g_host->getLogger && g_host->logNamed) {
        iec->setLogger(g_host->getLogger("VirtualIEC"), g_host->logNamed);
    }
    return iec;
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
    g_host = host;
    return &s_manifest;
}
