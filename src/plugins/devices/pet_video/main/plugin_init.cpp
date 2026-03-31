#include "include/mmemu_plugin_api.h"
#include "pet_video.h"
#include "libdevices/main/device_registry.h"

static IOHandler* createPetVideo2001() {
    return new PetVideo(PetVideo::Model::PET_2001);
}

static IOHandler* createPetVideo4000() {
    return new PetVideo(PetVideo::Model::PET_4000);
}

static IOHandler* createPetVideo8000() {
    return new PetVideo(PetVideo::Model::PET_8000);
}

static DevicePluginInfo s_devices[] = {
    { "pet_video_2001", createPetVideo2001 },
    { "pet_video_4000", createPetVideo4000 },
    { "pet_video_8000", createPetVideo8000 }
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "pet-video",
    "PET Video Subsystem",
    "1.0.0",
    nullptr, nullptr, // deps, supportedMachineIds
    0, nullptr,       // cores
    0, nullptr,       // toolchains
    3, s_devices,
    0, nullptr        // machines
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    (void)host;
    return &s_manifest;
}
