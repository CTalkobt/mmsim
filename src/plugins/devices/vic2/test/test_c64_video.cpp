#include "test_harness.h"
#include "vic2.h"
#include "io_registry.h"
#include "memory_bus.h"
#include "ibus.h"
#include "cpu6510.h"

#include <vector>
#include <cstring> // For memset

#define VIC2_BASE_ADDR 0xD000

TEST_CASE(C64VideoColorUpdate) {
    // --------------------------------------------------------------------
    // Memory and IORegistry setup
    // --------------------------------------------------------------------
    FlatMemoryBus systemBus("system", 16); // 64 KB memory space
    IORegistry ioRegistry;

    // Dummy ROMs and Color RAM for VIC-II
    std::vector<uint8_t> charRomVec(4096, 0);
    const uint8_t* charRom = charRomVec.data();
    std::vector<uint8_t> colorRamVec(1024, 0);
    const uint8_t* colorRam = colorRamVec.data();

    // --------------------------------------------------------------------
    // VIC-II setup
    // --------------------------------------------------------------------
    VIC2* vic2 = new VIC2();
    vic2->setDmaBus(&systemBus);
    vic2->setCharRom(charRom, charRomVec.size());
    vic2->setColorRam(colorRam);
    ioRegistry.registerOwnedHandler(vic2); // VIC2 constructor calls reset(), setting default colors

    // --------------------------------------------------------------------
    // CPU setup
    // --------------------------------------------------------------------
    MOS6510 cpu;
    cpu.setDataBus(&systemBus);
    cpu.setCodeBus(&systemBus);
    // cpu.setIoBus is not needed as IORegistry will dispatch I/O writes
    cpu.reset();

    // --------------------------------------------------------------------
    // Initial color check (should be VIC-II defaults)
    // --------------------------------------------------------------------
    uint8_t borderVal, bgVal;
    ioRegistry.dispatchRead(&systemBus, VIC2_BASE_ADDR + VIC2::EXTCOL, &borderVal);
    ioRegistry.dispatchRead(&systemBus, VIC2_BASE_ADDR + VIC2::BGCOL0, &bgVal);
    // Default values from VIC2::reset() in vic2.cpp:
    // m_regs[EXTCOL] = 0x0E; // light blue border (color index 14)
    // m_regs[BGCOL0] = 0x06; // blue background (color index 6)
    ASSERT_EQ(borderVal & 0x0F, 14); // Light Blue
    ASSERT_EQ(bgVal & 0x0F, 6);      // Blue

    // --------------------------------------------------------------------
    // Simulate cycles and update colors
    // --------------------------------------------------------------------
    const uint64_t TOTAL_CYCLES = 5000000;
    const uint64_t CHANGE_THRESHOLD = 1000000; // Change colors after 1 million cycles
    uint64_t cyclesRun = 0;
    bool colorsChanged = false;

    while (cyclesRun < TOTAL_CYCLES) {
        int cyclesThisStep = cpu.step();
        ioRegistry.tickAll(cyclesThisStep);
        cyclesRun += cyclesThisStep;

        if (!colorsChanged && cyclesRun >= CHANGE_THRESHOLD) {
            ioRegistry.dispatchWrite(&systemBus, VIC2_BASE_ADDR + VIC2::EXTCOL, 0x01); // White border (color index 1)
            ioRegistry.dispatchWrite(&systemBus, VIC2_BASE_ADDR + VIC2::BGCOL0, 0x01); // White background (color index 1)
            colorsChanged = true;
        }
    }

    // --------------------------------------------------------------------
    // Verify final colors
    // --------------------------------------------------------------------
    uint32_t frameBuffer[VIC2::FRAME_W * VIC2::FRAME_H];
    vic2->renderFrame(frameBuffer);

    // Get the white color from the VIC-II's internal palette
    uint32_t whiteColor = 0xFFFFFFFF;

    // Check a pixel in the border area (top-left corner)
    ASSERT_EQ(frameBuffer[0], whiteColor);

    // Check a pixel in the active display area (top-left of display area)
    ASSERT_EQ(frameBuffer[VIC2::DISPLAY_Y * VIC2::FRAME_W + VIC2::DISPLAY_X], whiteColor);
}
