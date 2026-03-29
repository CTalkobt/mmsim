#include "include/mmemu_plugin_api.h"
#include "libcore/main/image_loader.h"
#include "prg_loader.h"
#include "cbm_cart_handler.h"
#include <vector>

static const char* s_deps[] = { nullptr };
static const char* s_supportedMachines[] = { "vic20", "c64", "pet2001", "pet4032", "pet8032", nullptr };

static IImageLoader* createPrgLoader() {
    return new PrgLoader();
}

static ICartridgeHandler* createCbmCartridge(const char* path) {
    return new CbmCartridgeHandler(path);
}

static ImageLoaderPluginInfo s_loaders[] = {
    { createPrgLoader }
};

static CartridgePluginInfo s_cartridges[] = {
    { "crt", createCbmCartridge }
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "cbm-loader",
    "Commodore Image Loader",
    "1.0.0",
    s_deps,
    s_supportedMachines,
    0, nullptr, // Cores
    0, nullptr, // Toolchains
    0, nullptr, // Devices
    0, nullptr, // Machines
    1, s_loaders,
    1, s_cartridges
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    ImageLoaderRegistry::setInstance(host->imageLoaderRegistry);
    return &s_manifest;
}
