#include "test_harness.h"
#include "plugins/devices/vic4/main/vic4.h"
#include "plugins/devices/map_mmu/main/key_register.h"

TEST_CASE(vic4_personality_lock) {
    VIC4 vic;
    uint8_t val = 0;

    // Initially locked
    ASSERT(vic.isLocked());

    // Extended registers should read $FF when locked
    ASSERT(vic.ioRead(nullptr, 0xD040, &val));
    ASSERT_EQ(val, 0xFF);

    // Unlock
    vic.setLocked(false);
    ASSERT(!vic.isLocked());

    // Now should read internal state (default 0)
    ASSERT(vic.ioRead(nullptr, 0xD040, &val));
    ASSERT_EQ(val, 0x00);
}

TEST_CASE(vic4_extended_registers) {
    VIC4 vic;
    vic.setLocked(false);
    uint8_t val = 0;

    // Write to $D040
    vic.ioWrite(nullptr, 0xD040, 0x42);
    ASSERT(vic.ioRead(nullptr, 0xD040, &val));
    ASSERT_EQ(val, 0x42);

    // Write to $D07F
    vic.ioWrite(nullptr, 0xD07F, 0x7F);
    ASSERT(vic.ioRead(nullptr, 0xD07F, &val));
    ASSERT_EQ(val, 0x7F);
}

TEST_CASE(vic4_palette_access) {
    VIC4 vic;
    vic.setLocked(false);
    uint8_t val = 0;

    // $D101: Palette 1 Red
    vic.ioWrite(nullptr, 0xD101, 0xAA);
    ASSERT(vic.ioRead(nullptr, 0xD101, &val));
    ASSERT_EQ(val, 0xAA);

    // $D201: Palette 1 Green
    vic.ioWrite(nullptr, 0xD201, 0xBB);
    ASSERT(vic.ioRead(nullptr, 0xD201, &val));
    ASSERT_EQ(val, 0xBB);

    // $D301: Palette 1 Blue
    vic.ioWrite(nullptr, 0xD301, 0xCC);
    ASSERT(vic.ioRead(nullptr, 0xD301, &val));
    ASSERT_EQ(val, 0xCC);
}

TEST_CASE(vic4_key_integration) {
    VIC4 vic;
    KeyRegister keyReg;

    // Wire them via callback as in factory
    keyReg.setPersonalityChangeCallback([&vic](IopersonalityMode mode) {
        vic.setLocked(mode != IopersonalityMode::MEGA65);
    });

    ASSERT(vic.isLocked());

    // Switch to MEGA65 personality
    keyReg.ioWrite(nullptr, 0xD02F, 0x47);
    keyReg.ioWrite(nullptr, 0xD02F, 0x53);
    
    ASSERT(!vic.isLocked());

    // Switch back to C64
    keyReg.ioWrite(nullptr, 0xD02F, 0x00);
    keyReg.ioWrite(nullptr, 0xD02F, 0x00);

    ASSERT(vic.isLocked());
}
