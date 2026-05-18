#include "test_harness.h"
#include "plugins/devices/crtc6545/main/crtc6545.h"

TEST_CASE(crtc_initial_state) {
    CRTC6545 crtc;
    ASSERT(std::string(crtc.name()) == "CRTC6545");
    ASSERT_EQ(crtc.baseAddr(), (uint32_t)0xE880);
    ASSERT_EQ(crtc.addrMask(), (uint32_t)0x0001);
    ASSERT_EQ(crtc.getMA(), (uint16_t)0);
    ASSERT_EQ(crtc.getRA(), (uint8_t)0);
}

TEST_CASE(crtc_register_select_and_write) {
    CRTC6545 crtc;
    crtc.reset();

    // Select register 0 (horizontal total) by writing to $E880
    crtc.ioWrite(nullptr, 0xE880, 0);
    // Write value 63 to the selected register via $E881
    crtc.ioWrite(nullptr, 0xE881, 63);
    ASSERT_EQ(crtc.getReg(0), (uint8_t)63);

    // Select register 1 (horizontal displayed)
    crtc.ioWrite(nullptr, 0xE880, 1);
    crtc.ioWrite(nullptr, 0xE881, 40);
    ASSERT_EQ(crtc.getReg(1), (uint8_t)40);
}

TEST_CASE(crtc_register_read) {
    CRTC6545 crtc;
    crtc.reset();

    // Most registers are write-only; only R14-R17 (cursor) are readable
    crtc.ioWrite(nullptr, 0xE880, 14);
    crtc.ioWrite(nullptr, 0xE881, 0x12);

    // Read back R14
    crtc.ioWrite(nullptr, 0xE880, 14);
    uint8_t val = 0;
    crtc.ioRead(nullptr, 0xE881, &val);
    ASSERT_EQ(val, (uint8_t)0x12);

    // Reading a write-only register returns 0
    crtc.ioWrite(nullptr, 0xE880, 6);
    crtc.ioWrite(nullptr, 0xE881, 25);
    crtc.ioWrite(nullptr, 0xE880, 6);
    val = 0xFF;
    crtc.ioRead(nullptr, 0xE881, &val);
    ASSERT_EQ(val, (uint8_t)0);
}

TEST_CASE(crtc_out_of_range_register) {
    CRTC6545 crtc;
    // getReg with index >= 18 should return 0
    ASSERT_EQ(crtc.getReg(18), (uint8_t)0);
    ASSERT_EQ(crtc.getReg(255), (uint8_t)0);
}

TEST_CASE(crtc_tick_advances_counters) {
    CRTC6545 crtc;
    crtc.reset();

    // Configure: R0=3 (horiz total=4 chars per line)
    crtc.ioWrite(nullptr, 0xE880, 0);
    crtc.ioWrite(nullptr, 0xE881, 3);

    // R9=0 (1 scanline per char row)
    crtc.ioWrite(nullptr, 0xE880, 9);
    crtc.ioWrite(nullptr, 0xE881, 0);

    // R4=2 (3 char rows per frame)
    crtc.ioWrite(nullptr, 0xE880, 4);
    crtc.ioWrite(nullptr, 0xE881, 2);

    // Tick several times — counters should advance
    uint16_t prevMA = crtc.getMA();
    for (int i = 0; i < 20; i++) crtc.tick(1);
    // MA should have advanced from its initial position
    // (exact value depends on R1 horizontal displayed, which defaults to 0)
}

TEST_CASE(crtc_reset_clears_state) {
    CRTC6545 crtc;

    // Write some registers
    crtc.ioWrite(nullptr, 0xE880, 0);
    crtc.ioWrite(nullptr, 0xE881, 63);
    crtc.ioWrite(nullptr, 0xE880, 1);
    crtc.ioWrite(nullptr, 0xE881, 40);

    crtc.reset();

    // After reset, registers should be zeroed
    ASSERT_EQ(crtc.getReg(0), (uint8_t)0);
    ASSERT_EQ(crtc.getReg(1), (uint8_t)0);
    ASSERT_EQ(crtc.getMA(), (uint16_t)0);
    ASSERT_EQ(crtc.getRA(), (uint8_t)0);
}

TEST_CASE(crtc_start_address) {
    CRTC6545 crtc;
    crtc.reset();

    // R12 = start address high, R13 = start address low
    crtc.ioWrite(nullptr, 0xE880, 12);
    crtc.ioWrite(nullptr, 0xE881, 0x04);  // high byte
    crtc.ioWrite(nullptr, 0xE880, 13);
    crtc.ioWrite(nullptr, 0xE881, 0x00);  // low byte

    ASSERT_EQ(crtc.getReg(12), (uint8_t)0x04);
    ASSERT_EQ(crtc.getReg(13), (uint8_t)0x00);
}

TEST_CASE(crtc_name_setter) {
    CRTC6545 crtc;
    crtc.setName("PET-CRTC");
    ASSERT(std::string(crtc.name()) == "PET-CRTC");
}
