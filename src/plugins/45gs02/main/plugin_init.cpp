#include "mmemu_plugin_api.h"
#include "cpu45gs02.h"
#include "libcore/main/machine_desc.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/io_registry.h"
#include "hyper_serial.h"

static const SimPluginHostAPI* g_host = nullptr;

static ICore* createCore45GS02() {
    return new MOS45GS02();
}

static MachineDescriptor* createMachineRawMega65() {
    MachineDescriptor* desc = new MachineDescriptor();
    desc->machineId = "rawMega65";
    desc->displayName = "Raw MEGA65 (45GS02 / RAM only)";

    // 28-bit address bus for 256MB RAM
    FlatMemoryBus* bus = new FlatMemoryBus("system", 28);
    MOS45GS02* cpu = new MOS45GS02();
    cpu->setDataBus(bus);

    desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 40}); // 40MHz
    desc->buses.push_back({"system", bus});

    // Add Hyper Serial if available
    if (g_host && g_host->createDevice) {
        IOHandler* hs = g_host->createDevice("hyper_serial");
        if (hs) {
            hs->setBaseAddr(0xD6C0);
            if (!desc->ioRegistry) desc->ioRegistry = new IORegistry();
            desc->ioRegistry->registerOwnedHandler(hs);
            
            bus->setIoHooks(
                [io = desc->ioRegistry](IBus* b, uint32_t a, uint8_t* v) { return io->dispatchRead(b, a, v); },
                [io = desc->ioRegistry](IBus* b, uint32_t a, uint8_t v) { return io->dispatchWrite(b, a, v); }
            );

            bus->setHaltCheck([hs] {
                return static_cast<HyperSerialLogger*>(hs)->isHaltRequested();
            });
        }
    }

    return desc;
}

static CorePluginInfo s_cores[] = {
    {"45gs02", "MEGA65", "open", createCore45GS02}
};

static MachinePluginInfo s_machines[] = {
    {"rawMega65", createMachineRawMega65}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "45gs02",
    "MEGA65 45GS02 CPU Plugin",
    "0.1.0",
    nullptr, nullptr,
    1, s_cores,
    0, nullptr,
    0, nullptr,
    1, s_machines,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    return &s_manifest;
}
