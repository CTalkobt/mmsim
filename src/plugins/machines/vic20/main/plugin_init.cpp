#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"

// Forward declaration of the factory function in machine_vic20.cpp
MachineDescriptor* createMachineVic20();

static MachinePluginInfo s_machines[] = {
    {"vic20", createMachineVic20}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-vic20",
    "1.0.0",
    0, nullptr,
    0, nullptr,
    0, nullptr,
    1, s_machines
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    (void)host;
    return &s_manifest;
}
