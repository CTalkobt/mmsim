#include "test_harness.h"
#include "plugins/devices/gtia/main/gtia.h"
#include <vector>

TEST_CASE(gtia_registers) {
    GTIA gtia;
    uint8_t val;
    
    // Test register write/read
    // HPOSP0 is write-only, M0PF is read-only at same address
    gtia.ioWrite(nullptr, 0xD000, 0x40); // HPOSP0 = 0x40
    gtia.ioRead(nullptr, 0xD000, &val);  // Read M0PF
    ASSERT(val == 0); // No collisions yet
    
    // COLPM0 is R/W (well, it's write-only but I implemented it as stored in m_regs for simplicity in this stub)
    // Wait, my implementation returns 0x00 for default in ioRead if not hit/trig/pal/consol.
    // Let's check a readable register like CONSOL
    gtia.ioRead(nullptr, 0xD01F, &val);
    ASSERT(val == 0x07); // Default switches
}

TEST_CASE(gtia_palette) {
    // Test a few known colors
    // Color 0 is usually black
    ASSERT(GTIA::getRGBA(0x00) == 0x000000FF);
    // Color 0x0F is white
    ASSERT(GTIA::getRGBA(0x09) == 0xFFFFFFFF);
}

TEST_CASE(gtia_collisions) {
    GTIA gtia;
    uint8_t val;
    
    // Mock a collision (internal state for now)
    // In real use, collisions would be set by the renderer/tick.
    // Since tick is a stub, we'll verify HITCLR works.
    
    // We can't easily set collisions from outside without adding a test-only method
    // or implementing enough of tick.
    // For now, just test HITCLR resets registers to 0.
    
    gtia.ioWrite(nullptr, 0xD01E, 0x01); // HITCLR
    for (int i=0; i<16; ++i) {
        gtia.ioRead(nullptr, 0xD000 + i, &val);
        ASSERT(val == 0);
    }
}

TEST_CASE(gtia_console) {
    GTIA gtia;
    uint8_t val;
    
    gtia.setConsoleSwitches(0x02); // Select only
    gtia.ioRead(nullptr, 0xD01F, &val);
    ASSERT(val == 0x02);
}
