#include "mmemu_plugin_api.h"
#include <vector>
#include <string>

extern "C" {
    MachineDescriptor* createMachineAtari400();
    MachineDescriptor* createMachineAtari800();
    MachineDescriptor* createMachineAtari800XL();
}

extern const struct SimPluginHostAPI* g_atariHost;

namespace {
    const char* s_deps[] = { "6502", "antic", "gtia", "pokey", "pia6520", nullptr };

    MachinePluginInfo s_machines[] = {
        { "atari400",   createMachineAtari400 },
        { "atari800",   createMachineAtari800 },
        { "atari800xl", createMachineAtari800XL },
        { nullptr, nullptr }
    };

    SimPluginManifest s_manifest = {
        MMEMU_PLUGIN_API_VERSION,
        "atari-machines",
        "Atari 8-bit Machine Presets",
        "0.1.0",
        s_deps,
        nullptr, // supportedMachineIds (all)
        0, nullptr, // cores
        0, nullptr, // toolchains
        0, nullptr, // devices
        3, s_machines, // machines
        0, nullptr, // loaders
        0, nullptr  // cartridges
    };
}

extern "C" {
    SimPluginManifest* mmemuPluginInit(const struct SimPluginHostAPI* host) {
        g_atariHost = host;
        return &s_manifest;
    }

    void mmemuPluginCleanup() {
    }
}
