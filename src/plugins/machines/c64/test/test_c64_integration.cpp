#include "test_harness.h"
#include "libdevices/main/io_handler.h"
#include "libmem/main/ibus.h"
#include "plugins/devices/virtual_iec/main/virtual_iec.h"
#include <vector>
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// Test 11: IEC Integration (Standalone)
// ---------------------------------------------------------------------------

TEST_CASE(c64_iec_directory_load) {
    VirtualIECBus iec(8);

    std::string path = "tests/resources/1541-demo.d64";
    ASSERT(iec.mountDisk(8, path));

    auto writeBus = [&](uint8_t val) {
        iec.writePort(val);
        iec.tick(1000); // 1ms per state change
    };

    auto readBus = [&]() -> uint8_t {
        return iec.readPort();
    };

    // 1. Host pulls ATN low
    writeBus(0x08); 
    iec.tick(5000); 

    auto sendByte = [&](uint8_t byte) {
        for (int i = 0; i < 8; ++i) {
            bool bit = (byte >> i) & 1;
            writeBus(0x18 | (bit ? 0x20 : 0x00));
            iec.tick(500);
            writeBus(0x08 | (bit ? 0x20 : 0x00));
            iec.tick(500);
        }
        writeBus(0x08); // Release for ACK
        int timeout = 1000;
        while ((readBus() & 0x80) != 0 && --timeout > 0) iec.tick(100);
        ASSERT(timeout > 0);

        writeBus(0x18); // Host ACKs
        iec.tick(500);
        timeout = 1000;
        while ((readBus() & 0x80) == 0 && --timeout > 0) iec.tick(100);
        ASSERT(timeout > 0);
        writeBus(0x08);
        iec.tick(500);
    };

    sendByte(0x28); // LISTEN 8
    sendByte(0x60); // SECONDARY 0
    
    writeBus(0x00); // Release ATN
    iec.tick(2000);

    auto sendDataByte = [&](uint8_t byte) {
        // Listener (Device) should release CLK to signal ready
        int timeout = 5000;
        while ((readBus() & 0x40) == 0 && --timeout > 0) iec.tick(100);
        ASSERT(timeout > 0);

        for (int i = 0; i < 8; ++i) {
            bool bit = (byte >> i) & 1;
            // Talker (Host) pulls CLK low (bit 4 = 1)
            writeBus(0x10 | (bit ? 0x20 : 0x00));
            iec.tick(200);
            // Talker releases CLK (bit 4 = 0)
            writeBus(0x00 | (bit ? 0x20 : 0x00));
            iec.tick(200);
        }
        // Wait for ACK (Listener pulls DATA low)
        timeout = 5000;
        while ((readBus() & 0x80) != 0 && --timeout > 0) iec.tick(100);
        ASSERT(timeout > 0);
    };


    sendDataByte('$');  // Filename data
    
    iec.tick(5000);

    writeBus(0x08); // TALK sequence
    iec.tick(5000);
    sendByte(0x48); // TALK 8
    
    writeBus(0x00); // Release ATN
    iec.tick(1000); // ADDRESSING -> TURNAROUND -> TALK_READY
    writeBus(0x20); // Pull DATA (turnaround: host becomes listener)
    iec.tick(500);  // TALK_READY sub-phase 0 -> 1 (CLK released)

    auto readByte = [&]() -> uint8_t {
        uint8_t byte = 0;
        // Step 0: Pull DATA (listener present)
        writeBus(0x20);
        // Wait for CLK released (Step 1: talker ready to send)
        {
            int timeout = 5000;
            while ((readBus() & 0x40) == 0 && --timeout > 0) iec.tick(100);
            ASSERT(timeout > 0);
        }
        // Step 2: Release DATA (ready for data)
        writeBus(0x00);
        iec.tick(100);
        for (int i = 0; i < 8; ++i) {
            int timeout = 5000;
            while ((readBus() & 0x40) != 0 && --timeout > 0) iec.tick(100);
            ASSERT(timeout > 0);
            if (!(readBus() & 0x80)) byte |= (1 << i);
            timeout = 5000;
            while ((readBus() & 0x40) == 0 && --timeout > 0) iec.tick(100);
            ASSERT(timeout > 0);
        }
        // Frame handshake: pull DATA (byte accepted)
        writeBus(0x20);
        iec.tick(500);
        return byte;
    };

    uint8_t lo = readByte();
    uint8_t hi = readByte();
    ASSERT(lo == 0x01);
    ASSERT(hi == 0x08);
}
