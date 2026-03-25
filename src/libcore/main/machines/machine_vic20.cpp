#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "via6522.h"
#include "vic6560.h"
#include <iostream>

/**
 * VIC-20 Machine Descriptor Factory.
 */
static MachineDescriptor* createMachineVic20() {
    auto* desc = new MachineDescriptor();
    desc->machineId = "vic20";
    desc->displayName = "Commodore VIC-20";
    desc->licenseClass = "proprietary"; // ROMs are proprietary

    // 1. Memory Bus
    auto* bus = new FlatMemoryBus("system", 16); // 64 KB
    desc->buses.push_back({"system", bus});

    // 2. CPU
    ICore* cpu = CoreRegistry::instance().createCore("6502");
    if (cpu) {
        cpu->setDataBus(bus);
        cpu->setCodeBus(bus);
        desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 1});
    }

    // 3. I/O Devices
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    auto* vic = (VIC6560*)DeviceRegistry::instance().createDevice("6560");
    
    // Create VIA devices from registry (if available)
    auto* via1 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");
    auto* via2 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");

    if (!vic) vic = new VIC6560("VIC-I", 0x9000);
    else {
        vic->setName("VIC-I");
        vic->setBaseAddr(0x9000);
    }

    if (!via1 || !via2) {
        // Fallback to direct creation if plugin not loaded or internal
        if (!via1) via1 = new VIA6522("VIA1", 0x9110);
        if (!via2) via2 = new VIA6522("VIA2", 0x9120);
    } else {
        via1->setName("VIA1");
        via1->setBaseAddr(0x9110);
        via2->setName("VIA2");
        via2->setBaseAddr(0x9120);
    }

    io->registerHandler(vic);
    io->registerHandler(via1);
    io->registerHandler(via2);

    // TODO: Wire interrupts (VIA -> CPU IRQ)
    // TODO: Connect Colour RAM at $9400 and $9600

    // 4. ROM Overlays (Placeholders)
    // desc->overlays.push_back({"roms/vic20/char.bin", 0x8000, true, true});
    // desc->overlays.push_back({"roms/vic20/basic.bin", 0xC000, true, true});
    // desc->overlays.push_back({"roms/vic20/kernal.bin", 0xE000, true, true});

    desc->onReset = [](MachineDescriptor& d) {
        if (d.ioRegistry) d.ioRegistry->resetAll();
        for (auto& slot : d.cpus) {
            if (slot.cpu) slot.cpu->reset();
        }
    };

    desc->schedulerStep = [](MachineDescriptor& d) {
        int totalCycles = 0;
        for (auto& slot : d.cpus) {
            if (slot.active && slot.cpu) {
                int cycles = slot.cpu->step();
                totalCycles += cycles;
                if (d.ioRegistry) d.ioRegistry->tickAll(cycles);
            }
        }
        return totalCycles;
    };

    return desc;
}

/**
 * Register the VIC-20 machine at startup.
 */
static bool s_vic20Registered = []() {
    MachineRegistry::instance().registerMachine("vic20", createMachineVic20);
    return true;
}();
