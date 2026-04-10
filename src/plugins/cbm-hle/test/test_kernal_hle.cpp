#include "test_harness.h"
#include "plugins/cbm-hle/main/kernal_hle.h"
#include "plugins/6502/main/cpu6502.h"
#include "libmem/main/memory_bus.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE(kernal_hle_load) {
    MOS6502 cpu;
    FlatMemoryBus bus("system", 16);
    cpu.setDataBus(&bus);
    
    KernalHLE hle;
    hle.setHostPath(".");
    
    // Create a dummy file
    std::string filename = "test.prg";
    {
        std::ofstream f(filename, std::ios::binary);
        uint8_t header[] = { 0x00, 0x20 }; // Load at $2000
        f.write((char*)header, 2);
        uint8_t data[] = { 0xAD, 0x00, 0x20, 0x60 }; // LDA $2000, RTS
        f.write((char*)data, sizeof(data));
    }
    
    // Set up KERNAL variables in RAM
    bus.write8(0xBA, 8); // Device 8
    bus.write8(0xB9, 1); // Secondary Address 1 (use header)
    bus.write8(0xB7, filename.length()); // Filename length
    uint16_t namePtr = 0x0200;
    bus.write8(0xBB, namePtr & 0xFF);
    bus.write8(0xBC, namePtr >> 8);
    for (size_t i = 0; i < filename.length(); ++i) {
        bus.write8(namePtr + i, filename[i]);
    }
    
    // Set up stack for RTS simulation
    cpu.regWriteByName("SP", 0xFD);
    bus.write8(0x01FE, 0x00); // Return address lo
    bus.write8(0x01FF, 0x10); // Return address hi ($1000)
    
    // Trigger HLE at $FFD5
    DisasmEntry entry;
    entry.addr = 0xFFD5;
    entry.mnemonic = "JSR";
    
    bool cont = hle.onStep(&cpu, &bus, entry);
    ASSERT(cont == false); // Should have intercepted
    
    // Verify data injected at $2000
    ASSERT(bus.read8(0x2000) == 0xAD);
    ASSERT(bus.read8(0x2001) == 0x00);
    ASSERT(bus.read8(0x2002) == 0x20);
    ASSERT(bus.read8(0x2003) == 0x60);
    
    // Verify registers updated
    // New X/Y should be end address ($2004)
    ASSERT(cpu.regReadByName("X") == 0x04);
    ASSERT(cpu.regReadByName("Y") == 0x20);
    
    // Verify Carry flag cleared (success)
    ASSERT((cpu.regReadByName("P") & 0x01) == 0);
    
    // Verify PC updated to return address ($1000 + 1 = $1001)
    ASSERT(cpu.pc() == 0x1001);
    
    // Cleanup
    fs::remove(filename);
}
