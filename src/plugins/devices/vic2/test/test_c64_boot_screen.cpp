#include "test_harness.h"
#include "vic2.h"
#include "io_registry.h"
#include "memory_bus.h"
#include "ibus.h"
#include "cpu6510.h"
#include "rom_loader.h" // For romLoad

#include <vector>
#include <cstring> // For memset
#include <fstream> // For reading ROM files
#include <string> // For std::string
#include <iostream> // For debugging, remove later

#define VIC2_BASE_ADDR 0xD000
#define SCREEN_RAM_START 0x0400
#define SCREEN_RAM_SIZE 1000 // 40 cols * 25 rows

// Helper function to read ROM files
std::vector<uint8_t> readRomFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file: " << path << std::endl;
        return {}; // Return empty vector if file not found
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (file.read((char*)buffer.data(), size)) {
        return buffer;
    }
    std::cerr << "Error: Could not read ROM file: " << path << std::endl;
    return {};
}

TEST_CASE(C64BootScreenContent) {
    // --------------------------------------------------------------------
    // Memory and IORegistry setup
    // --------------------------------------------------------------------
    FlatMemoryBus systemBus("system", 16); // 64 KB memory space
    IORegistry ioRegistry;

    // Load ROM files
    std::vector<uint8_t> basicRom = readRomFile("roms/c64/basic.bin");
    std::vector<uint8_t> kernalRom = readRomFile("roms/c64/kernal.bin");
    std::vector<uint8_t> charRom = readRomFile("roms/c64/char.bin");

    // Ensure ROMs are actually loaded.
    ASSERT(basicRom.size() > 0);
    ASSERT(kernalRom.size() > 0);
    ASSERT(charRom.size() > 0);

    // romLoad(bus, name, addr, size, data, writable)
    systemBus.addOverlay(0xA000, basicRom.size(), basicRom.data(), false);
    systemBus.addOverlay(0xE000, kernalRom.size(), kernalRom.data(), false);
    // Character ROM is provided directly to VIC-II, not mapped directly to CPU bus
    // for standard operation, but is used by VIC-II's DMA.

    // Initialize RAM (all zeros typically)
    std::vector<uint8_t> ram(0x10000, 0); // 64KB RAM
    systemBus.addOverlay(0x0000, 0x10000, ram.data(), true);

    // Dummy Color RAM for VIC-II. VIC-II needs to be able to write to it.
    std::vector<uint8_t> colorRamVec(1024, 0); // Initialize with 0s (black)
    systemBus.addOverlay(0xD800, 1024, colorRamVec.data(), true);

    // --------------------------------------------------------------------
    // VIC-II setup
    // --------------------------------------------------------------------
    VIC2* vic2 = new VIC2();
    vic2->setDmaBus(&systemBus); // VIC-II uses systemBus for DMA
    vic2->setCharRom(charRom.data(), charRom.size());
    // VIC-II will access color RAM through its DMA bus (systemBus) via its address
    ioRegistry.registerOwnedHandler(vic2); // IORegistry owns and deletes vic2

    // --------------------------------------------------------------------
    // CPU setup
    // --------------------------------------------------------------------
    MOS6510 cpu;
    cpu.setDataBus(&systemBus);
    cpu.setCodeBus(&systemBus);
    cpu.reset(); // This will read reset vector from KERNAL ROM

    // --------------------------------------------------------------------
    // Simulate boot sequence
    // --------------------------------------------------------------------
    const uint64_t BOOT_CYCLES = 500000; // Enough cycles for boot to READY.
    uint64_t cyclesRun = 0;

    while (cyclesRun < BOOT_CYCLES) {
        int cyclesThisStep = cpu.step();
        ioRegistry.tickAll(cyclesThisStep);
        cyclesRun += cyclesThisStep;
    }

    // Verify screen content
    // --------------------------------------------------------------------
    bool nonSpaceFound = false;
    for (uint16_t addr = SCREEN_RAM_START; addr < SCREEN_RAM_START + SCREEN_RAM_SIZE; ++addr) {
        uint8_t charCode = systemBus.read8(addr); 
        if (charCode != 0x20) { // 0x20 is ASCII space
            nonSpaceFound = true;
            break;
        }
    }
    ASSERT(nonSpaceFound); // Assert that at least one non-space character was found in screen RAM

    // --------------------------------------------------------------------
    // Verify pixels are drawn on screen
    // --------------------------------------------------------------------
    vic2->exportPng("c64_boot_screen.png"); // Save a screenshot

    // Get the frame buffer
    auto dims = vic2->getDimensions();
    std::vector<uint32_t> frameBuffer(dims.width * dims.height);
    vic2->renderFrame(frameBuffer.data());

    // Check for non-background pixels
    bool nonBackgroundPixelFound = false;
    uint32_t backgroundColor = 0x0000AAFF; // C64 blue
    for (uint32_t pixel : frameBuffer) {
        if (pixel != backgroundColor) {
            nonBackgroundPixelFound = true;
            break;
        }
    }
    ASSERT(nonBackgroundPixelFound); // Assert that at least one pixel is not the background color

}