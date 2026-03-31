#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include <vector>

// Forward declarations of factory functions in machine_vic20.cpp
MachineDescriptor* createMachineVic20();
MachineDescriptor* createMachineVic20_3K();
MachineDescriptor* createMachineVic20_8K();
MachineDescriptor* createMachineVic20_16K();
MachineDescriptor* createMachineVic20_32K();

static MachinePluginInfo s_machines[] = {
    {"vic20",      createMachineVic20},
    {"vic20+3k",   createMachineVic20_3K},
    {"vic20+8k",   createMachineVic20_8K},
    {"vic20+16k",  createMachineVic20_16K},
    {"vic20+32k",  createMachineVic20_32K},
};

static const char* s_vic20Deps[] = { "6502", "via6522", "vic6560", "kbd-vic20", "cbm-loader", nullptr };

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "vic20",
    nullptr,
    "1.0.0",
    s_vic20Deps,
    nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr,
    5, s_machines,
    0, nullptr,
    0, nullptr
};

extern const SimPluginHostAPI* g_vic20Host;

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    auto** ptr = const_cast<const SimPluginHostAPI**>(&g_vic20Host);
    *ptr = host;
    return &s_manifest;
}
