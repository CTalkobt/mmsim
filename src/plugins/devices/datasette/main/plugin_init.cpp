#include "mmemu_plugin_api.h"
#include "datasette.h"

static IOHandler* createDatasette() {
    return new Datasette();
}

static DevicePluginInfo s_devices[] = {
    {"datasette", createDatasette}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "datasette",
    "Commodore Datasette (tape)",
    "1.0.0",
    nullptr,
    nullptr,
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
