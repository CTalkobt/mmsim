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

#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libtoolchain/main/toolchain_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->coreRegistry) CoreRegistry::setInstance(host->coreRegistry);
    if (host->machineRegistry) MachineRegistry::setInstance(host->machineRegistry);
    if (host->deviceRegistry) DeviceRegistry::setInstance(host->deviceRegistry);
    if (host->toolchainRegistry) ToolchainRegistry::setInstance(host->toolchainRegistry);
    return &s_manifest;
}
