#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/ivideo_output.h"
#include "vic_display_pane.h"
#include <vector>

// Forward declaration of factory in machine_c64.cpp
MachineDescriptor* createMachineC64();

static MachinePluginInfo s_machines[] = {
    {"c64", createMachineC64},
};

// ---------------------------------------------------------------------------
// C64 display pane — reuses VicDisplayPane (generic IVideoOutput renderer).
// Wired to the first IVideoOutput found in the machine's IORegistry (VIC-II).
// ---------------------------------------------------------------------------

static const char* s_c64Ids[] = {"c64", nullptr};

static void* createC64DisplayPane(void* parentHandle, void* /*ctx*/) {
    auto* parent = static_cast<wxWindow*>(parentHandle);
    return static_cast<void*>(new VicDisplayPane(parent));
}

static void onC64MachineLoad(void* paneHandle, MachineDescriptor* desc, void* /*ctx*/) {
    auto* pane = static_cast<VicDisplayPane*>(paneHandle);
    IVideoOutput* vid = nullptr;
    if (desc && desc->ioRegistry) {
        std::vector<IOHandler*> handlers;
        desc->ioRegistry->enumerate(handlers);
        for (auto* h : handlers) {
            if ((vid = dynamic_cast<IVideoOutput*>(h))) break;
        }
    }
    pane->SetVideoOutput(vid);
}

static void refreshC64DisplayPane(void* paneHandle, uint64_t /*cycles*/, void* /*ctx*/) {
    static_cast<VicDisplayPane*>(paneHandle)->RefreshFrame();
}

static PluginPaneInfo s_c64ScreenPane = {
    "c64.screen",
    "Screen",
    nullptr,
    s_c64Ids,
    createC64DisplayPane,
    nullptr,                // destroyPane: notebook DeletePage handles the window
    refreshC64DisplayPane,
    onC64MachineLoad,
    nullptr                 // ctx
};

// ---------------------------------------------------------------------------

static const char* s_c64Deps[] = { "6502", "c64-pla", "cia6526", "vic2", "sid6581", "cbm-loader", nullptr };

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "c64",
    nullptr,        // displayName
    "1.0.0",
    s_c64Deps,      // deps
    nullptr,        // supportedMachineIds
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
    if (host->coreRegistry)     CoreRegistry::setInstance(host->coreRegistry);
    if (host->machineRegistry)  MachineRegistry::setInstance(host->machineRegistry);
    if (host->deviceRegistry)   DeviceRegistry::setInstance(host->deviceRegistry);
    if (host->toolchainRegistry) ToolchainRegistry::setInstance(host->toolchainRegistry);
    if (host->registerPane)     host->registerPane(&s_c64ScreenPane);
    return &s_manifest;
}
