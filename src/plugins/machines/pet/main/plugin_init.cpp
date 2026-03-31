#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include <vector>

// Factory functions from machine_pet.cpp
MachineDescriptor* createPet2001();
MachineDescriptor* createPet4032();
MachineDescriptor* createPet8032();

static MachinePluginInfo s_machines[] = {
    { "pet2001", createPet2001 },
    { "pet4032", createPet4032 },
    { "pet8032", createPet8032 }
};

static const char* s_deps[] = { "6502", "pia6520", "via6522", "crtc6545", "pet-video", "cbm-loader", nullptr };

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "pet",
    "Commodore PET Series",
    "1.0.0",
    s_deps,
    nullptr, // supportedMachineIds
    0, nullptr,
    0, nullptr,
    0, nullptr,
    3, s_machines,
    0, nullptr,
    0, nullptr
};

extern const SimPluginHostAPI* g_petHost;

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    auto** ptr = const_cast<const SimPluginHostAPI**>(&g_petHost);
    *ptr = host;
    return &s_manifest;
}
