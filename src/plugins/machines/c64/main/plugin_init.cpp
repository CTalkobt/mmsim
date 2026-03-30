#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include <vector>

// Forward declarations
MachineDescriptor* createMachineC64();

static MachinePluginInfo s_machines[] = {
    {"c64", createMachineC64}
};

static const char* s_c64Deps[] = { "6502", "c64-pla", "cia6526", "vic2", "sid6581", "cbm-loader", nullptr };

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "c64",
    nullptr,
    "1.0.0",
    s_c64Deps,
    nullptr,
    0, nullptr,
    0, nullptr,
    0, nullptr,
    1, s_machines,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->coreRegistry) CoreRegistry::setInstance(host->coreRegistry);
    if (host->machineRegistry) MachineRegistry::setInstance(host->machineRegistry);
    if (host->deviceRegistry)   DeviceRegistry::setInstance(host->deviceRegistry);
    if (host->toolchainRegistry) ToolchainRegistry::setInstance(host->toolchainRegistry);
    return &s_manifest;
}
