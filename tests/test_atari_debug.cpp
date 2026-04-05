#include "test_harness.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/ibus.h"
#include "libdevices/main/io_registry.h"
#include "mmemu_plugin_api.h"
#include <iostream>
#include <iomanip>

// Defined in machine_atari.cpp
extern "C" MachineDescriptor* createMachineAtari400();
extern "C" MachineDescriptor* createMachineAtari800();
extern "C" MachineDescriptor* createMachineAtari800XL();

static IBus* getBus(MachineDescriptor* desc) {
    for (auto& b : desc->buses) {
        if (b.busName == "system") return b.bus;
    }
    return nullptr;
}

static void run_atari_boot_debug(MachineDescriptor* desc, const std::string& name) {
    auto* bus = getBus(desc);
    auto* cpu = desc->cpus[0].cpu;

    uint16_t resetVec = bus->peek8(0xFFFC) | (bus->peek8(0xFFFD) << 8);
    std::cerr << "--- Debugging Boot for " << name << " ---" << std::endl;
    std::cerr << "Reset Vector: $" << std::hex << resetVec << std::endl;

    if (desc->onReset) desc->onReset(*desc);

    // Run for a bit and log PC
    for (int i = 0; i < 1000; ++i) {
        uint16_t pc = (uint16_t)cpu->regReadByName("PC");
        uint8_t op = bus->read8(pc);
        // std::cerr << "PC: $" << std::hex << pc << " OP: $" << (int)op << std::endl;
        desc->schedulerStep(*desc);
        if (cpu->isHalted()) {
             std::cerr << "CPU HALTED at PC=$" << std::hex << pc << std::endl;
             break;
        }
    }

    uint16_t pc = (uint16_t)cpu->regReadByName("PC");
    std::cerr << "PC after 1000 steps: $" << std::hex << pc << std::endl;

    // Check RAM size variables
    uint8_t ramsiz = bus->read8(0x02E4);
    uint8_t ramlo = bus->read8(0x0006);
    std::cerr << "RAMSIZ ($02E4): " << std::dec << (int)ramsiz << " pages (" << (ramsiz * 256 / 1024) << " KB)" << std::endl;
    std::cerr << "RAM Limit ($06): $" << std::hex << (int)ramlo << std::endl;

    delete desc;
}

TEST_CASE(atari800_boot_debug) {
    run_atari_boot_debug(createMachineAtari800(), "Atari 800");
}

TEST_CASE(atari800xl_boot_debug) {
    run_atari_boot_debug(createMachineAtari800XL(), "Atari 800XL");
}
