#include "test_harness.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/icore.h"
#include "libmem/main/ibus.h"
#include "libdevices/main/io_registry.h"
#include "mmemu_plugin_api.h"
#include <iostream>

// Defined in machine_atari.cpp
extern "C" MachineDescriptor* createMachineAtari400();

static IBus* getBus(MachineDescriptor* desc) {
    for (auto& b : desc->buses) {
        if (b.busName == "system") return b.bus;
    }
    return nullptr;
}

TEST_CASE(atari400_boots_past_io_clearing) {
    auto* desc = createMachineAtari400();
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);
    ASSERT(bus != nullptr);
    auto* cpu = desc->cpus[0].cpu;
    ASSERT(cpu != nullptr);

    cpu->reset();

    // Run for 10000 cycles to get past RAM sizing and I/O clearing
    for (int i=0; i<10000; ++i) {
        cpu->step();
    }

    // For 16KB machine, RAMTOP should be 64 ($40)
    uint8_t ramtop = bus->read8(0x02E4);
    printf("RAMTOP at $02E4 is $%02x\n", ramtop);
    ASSERT(ramtop == 0x40);

    delete desc;
}
