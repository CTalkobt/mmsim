#include "mmemu_plugin_api.h"
#include "kernal_hle.h"
#include <vector>
#include <string>

static KernalHLE* s_hle = nullptr;

static int cmdHle(int argc, const char* const* argv, void* ctx) {
    if (argc < 2) {
        printf("Usage: hle <on|off|path <path>>\n");
        return 1;
    }

    std::string sub = argv[1];
    if (sub == "on") {
        s_hle->setEnabled(true);
        printf("KERNAL HLE enabled.\n");
    } else if (sub == "off") {
        s_hle->setEnabled(false);
        printf("KERNAL HLE disabled.\n");
    } else if (sub == "path" && argc >= 3) {
        s_hle->setHostPath(argv[2]);
        printf("KERNAL HLE host path set to: %s\n", argv[2]);
    } else {
        printf("Unknown hle subcommand.\n");
        return 1;
    }
    return 0;
}

static PluginCommandInfo s_commands[] = {
    {"hle", "on|off|path <path> - Configure KERNAL HLE", cmdHle, nullptr}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "cbm-hle",
    "CBM KERNAL HLE",
    "1.0.0",
    nullptr,        // deps
    nullptr,        // supportedMachineIds (all CBM machines handled in code)
    0, nullptr,     // cores
    0, nullptr,     // toolchains
    0, nullptr,     // devices
    0, nullptr,     // machines
    0, nullptr,     // loaders
    0, nullptr      // cartridges
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    s_hle = new KernalHLE();
    
    // Register the observer
    host->registerObserver(s_hle);
    
    // Register commands
    for (auto& cmd : s_commands) {
        host->registerCommand(&cmd);
    }
    
    return &s_manifest;
}
