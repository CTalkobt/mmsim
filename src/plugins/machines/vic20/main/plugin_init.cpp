#include "mmemu_plugin_api.h"
#include "libcore/main/json_machine_loader.h"

static const SimPluginHostAPI* g_host = nullptr;

static const char* s_vic20Deps[] = { "6502", "via6522", "vic6560", "kbd-vic20", "cbm-loader", nullptr };

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "vic20",
    "Commodore VIC-20",
    "1.0.0",
    s_vic20Deps,
    nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    JsonMachineLoader loader;
    loader.loadFile("machines/vic20.json");
    return &s_manifest;
}
