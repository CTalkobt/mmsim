#include "mmemu_plugin_api.h"
#include "hyper_serial.h"

static IOHandler* createHyperSerial() {
    return new HyperSerialLogger();
}

static DevicePluginInfo s_devices[] = {
    {"hyper_serial", createHyperSerial}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "hyper_serial",
    "MEGA65 Hyper Serial Logger",
    "0.1.0",
    nullptr, nullptr,
    0, nullptr,
    0, nullptr,
    1, s_devices,
    0, nullptr,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    return &s_manifest;
}
