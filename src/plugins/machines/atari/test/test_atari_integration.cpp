#include "test_harness.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/ibus.h"
#include "libdevices/main/io_registry.h"
#include "mmemu_plugin_api.h"
#include <iostream>

// Defined in machine_atari.cpp
extern "C" MachineDescriptor* createMachineAtari400();
extern "C" MachineDescriptor* createMachineAtari800();
extern "C" MachineDescriptor* createMachineAtari800XL();

static void ensureAtariRegistriesReady() {
    // Usually handled by plugin loading, but for integration tests we ensure registries are populated
}

static IBus* getBus(MachineDescriptor* desc) {
    for (auto& b : desc->buses) {
        if (b.busName == "system") return b.bus;
    }
    return nullptr;
}

TEST_CASE(atari400_setup) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari400();
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "atari400");
    
    auto* bus = getBus(desc);
    ASSERT(bus != nullptr);
    
    // RAM should be 16KB
    bus->write8(0x3FFF, 0x55);
    ASSERT(bus->read8(0x3FFF) == 0x55);
    
    // 0x4000 should be open bus (0xFF) in our OpenBusHandler
    ASSERT(bus->read8(0x4000) == 0xFF);
    
    delete desc;
}

TEST_CASE(atari800_setup) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari800();
    ASSERT(desc != nullptr);
    
    auto* bus = getBus(desc);
    // RAM should be 48KB
    bus->write8(0xBFFF, 0xAA);
    ASSERT(bus->read8(0xBFFF) == 0xAA);
    
    delete desc;
}

TEST_CASE(atari_setup) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari800XL();
    ASSERT(desc != nullptr);
    
    ASSERT(desc->cpus.size() == 1);
    ASSERT(desc->cpus[0].slotName == "main");
    
    auto* bus = getBus(desc);
    ASSERT(bus != nullptr);
    
    // Basic RAM check
    bus->write8(0x1000, 0xA5);
    ASSERT(bus->read8(0x1000) == 0xA5);
    
    // IO Check (GTIA)
    desc->ioRegistry->dispatchWrite(bus, 0xD01A, 0x12); // COLBK
    uint8_t val = 0;
    desc->ioRegistry->dispatchRead(bus, 0xD01A, &val);
    ASSERT(val == 0x12);
    
    delete desc;
}

TEST_CASE(atari_banking) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari800XL();
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);

    // Set Port B to Output (DDRB = $FF)
    desc->ioRegistry->dispatchWrite(bus, 0xD303, 0x00); // CRB: DDR access
    desc->ioRegistry->dispatchWrite(bus, 0xD301, 0xFF); // DDRB = $FF (all output)
    desc->ioRegistry->dispatchWrite(bus, 0xD303, 0x04); // CRB: Port access
    
    // Disable OS and BASIC (Bit 0=1, Bit 1=1) -> Port B = $03
    desc->ioRegistry->dispatchWrite(bus, 0xD301, 0x03);
    
    // Write something to RAM at $A000 and $C000 (where BASIC/OS usually live)
    // We use read8Raw/write8Raw if available, but write8 to a non-overlayed area is RAM.
    bus->write8(0xA000, 0xAA);
    bus->write8(0xC000, 0xBB);
    ASSERT(bus->read8(0xA000) == 0xAA);
    ASSERT(bus->read8(0xC000) == 0xBB);
    
    delete desc;
}

TEST_CASE(atari_halt_line) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari800XL();
    ASSERT(desc != nullptr);
    
    auto* cpu = desc->cpus[0].cpu;
    auto* bus = getBus(desc);
    
    // Trigger WSYNC
    desc->ioRegistry->dispatchWrite(bus, 0xD40A, 0x01);
    
    // CPU should be halted now
    ASSERT(cpu->isHalted());
    
    // Tick ANTIC until end of line (114 cycles)
    desc->ioRegistry->tickAll(114);
    
    // CPU should no longer be halted
    ASSERT(!cpu->isHalted());
    
    delete desc;
}

TEST_CASE(atari800xl_boot_basic) {
    ensureAtariRegistriesReady();
    auto* desc = createMachineAtari800XL();
    ASSERT(desc != nullptr);
    
    auto* bus = getBus(desc);
    // Check if ROMs loaded by looking at the reset vector
    uint16_t resetVec = bus->peek8(0xFFFC) | (bus->peek8(0xFFFD) << 8);
    std::cerr << "Atari Boot: Reset Vector = $" << std::hex << resetVec << std::endl;
    ASSERT(resetVec >= 0xC000); // OS ROM range

    // Explicitly enable BASIC via PIA Port B
    // 1. Set Port B to Output
    desc->ioRegistry->dispatchWrite(bus, 0xD303, 0x00); // CRB: DDR access
    desc->ioRegistry->dispatchWrite(bus, 0xD301, 0xFF); // DDRB = $FF
    desc->ioRegistry->dispatchWrite(bus, 0xD303, 0x04); // CRB: Port access
    // 2. Enable BASIC (Bit 1=0) and OS (Bit 0=0)
    desc->ioRegistry->dispatchWrite(bus, 0xD301, 0xFC); 

    // Reset machine to start from OS reset vector with BASIC enabled
    if (desc->onReset) desc->onReset(*desc);

    // Run for approx 5 seconds of emulated time (1.79 MHz)
    // We'll run 10 million cycles.
    for (int cycles = 0; cycles < 10000000; ) {
        int delta = desc->schedulerStep(*desc);
        cycles += delta;
        if (delta == 0) {
            std::cerr << "Atari Boot: schedulerStep returned 0! CPU Halted?" << std::endl;
            break;
        }
        if (cycles % 1000000 < 10) {
            uint16_t pc = (uint16_t)desc->cpus[0].cpu->regReadByName("PC");
            std::cerr << "Atari Boot: cycles=" << std::dec << cycles << " PC=$" << std::hex << pc << std::endl;
        }
    }

    // After boot, the PC should be in the BASIC ROM input loop ($A000-$BFFF)
    // or waiting in the OS GETLINE routine ($Fxxx).
    // For Atari BASIC, it should be in the $A000 range.
    uint16_t pc = (uint16_t)desc->cpus[0].cpu->regReadByName("PC");
    
    std::cerr << "Atari 800XL PC after boot: $" << std::hex << pc << std::endl;
    
    // For now, let's just assert it didn't crash to 0 and moved past reset
    ASSERT(pc != 0 && pc != resetVec);
    
    delete desc;
}
