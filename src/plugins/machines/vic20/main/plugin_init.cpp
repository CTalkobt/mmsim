#include "mmemu_plugin_api.h"
#include "libcore/main/machine_desc.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/ivideo_output.h"
#include "vic_display_pane.h"
#include <vector>

// Forward declaration of the factory function in machine_vic20.cpp
MachineDescriptor* createMachineVic20();

static MachinePluginInfo s_machines[] = {
    {"vic20", createMachineVic20}
};

// ---------------------------------------------------------------------------
// VIC display pane — registered as a plugin pane, shown for "vic20" machines.
// ---------------------------------------------------------------------------

static const char* s_vic20Ids[] = {"vic20", nullptr};

static void* createVicDisplayPane(void* parentHandle, void* /*ctx*/) {
    auto* parent = static_cast<wxWindow*>(parentHandle);
    auto* pane = new VicDisplayPane(parent);
    return static_cast<void*>(pane);
}

static void onVicMachineLoad(void* paneHandle, MachineDescriptor* desc, void* /*ctx*/) {
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

static void refreshVicDisplayPane(void* paneHandle, uint64_t /*cycles*/, void* /*ctx*/) {
    static_cast<VicDisplayPane*>(paneHandle)->RefreshFrame();
}

static PluginPaneInfo s_vicScreenPane = {
    "vic20.screen",
    "Screen",
    nullptr,
    s_vic20Ids,
    createVicDisplayPane,
    nullptr,                // destroyPane: notebook DeletePage handles the window
    refreshVicDisplayPane,
    onVicMachineLoad,
    nullptr                 // ctx
};

// ---------------------------------------------------------------------------

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-vic20",
    nullptr,        // displayName
    "1.0.0",
    nullptr,        // deps
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
    if (host->coreRegistry) CoreRegistry::setInstance(host->coreRegistry);
    if (host->machineRegistry) MachineRegistry::setInstance(host->machineRegistry);
    if (host->deviceRegistry) DeviceRegistry::setInstance(host->deviceRegistry);
    if (host->toolchainRegistry) ToolchainRegistry::setInstance(host->toolchainRegistry);
    if (host->registerPane) host->registerPane(&s_vicScreenPane);
    return &s_manifest;
}
