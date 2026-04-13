#include "test_harness.h"
#include "virtual_iec.h"
#include <fstream>
#include <cstdio>

TEST_CASE(virtual_iec_handshake) {
    VirtualIECBus iec(8);
    
    // Initial state: all lines released (Voltage High = bit 0 after inverters)
    iec.writePort(0x00);
    // bits 3 (ATN), 6 (CLK) and 7 (DATA) should be 0
    ASSERT((iec.readPort() & 0xC8) == 0x00);
    
    // Host pulls ATN (bit 3 = 1)
    iec.writePort(0x08);
    
    // Tick to detect ATN
    iec.tick(100); 
    // Tick again to allow timer to pass 1000
    iec.tick(2000);
    
    // Device should respond by pulling DATA low (bit 7 in readPort becomes 1 after inverters)
    uint8_t val = iec.readPort();
    ASSERT((val & 0x80) != 0);
    
    // Host sends a byte (e.g. LISTEN 8 = 0x28)
    uint8_t cmd = 0x28;
    for (int i = 0; i < 8; ++i) {
        bool bit = (cmd >> i) & 1;
        // Host pulls CLK low (bit 4 = 1)
        iec.writePort(0x18 | (bit ? 0x20 : 0x00));
        iec.tick(100);
        // Host releases CLK (bit 4 = 0)
        iec.writePort(0x08 | (bit ? 0x20 : 0x00));
        iec.tick(100);
    }
    
    // After 8 bits, device pulls DATA low to acknowledge byte (bit 7 = 1)
    val = iec.readPort();
    ASSERT((val & 0x80) != 0);
}

TEST_CASE(virtual_iec_disk_status) {
    VirtualIECBus iec(8);
    int t, s;
    bool led;
    
    // Initial status
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(t == 0);
    ASSERT(s == 0);
    ASSERT(led == false);
    
    // Mount a dummy file
    const char* dummyPath = "test_dummy.prg";
    std::ofstream f(dummyPath, std::ios::binary);
    f << "dummy data";
    f.close();
    
    ASSERT(iec.mountDisk(8, dummyPath));
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(t == 18);
    ASSERT(led == false); 
    
    // Busy during tick
    iec.writePort(0x08); // ATN
    iec.tick(100);
    iec.tick(3000);
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(led == true);
    
    // Idle after reset
    iec.reset();
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(led == false);
    
    std::remove(dummyPath);
}

TEST_CASE(virtual_iec_g64_mount) {
    VirtualIECBus iec(8);
    const char* g64Path = "test.g64";
    std::ofstream f(g64Path, std::ios::binary);
    f << "GCR-1541";
    f.write("\x00\x00\x00\x00", 4);
    f.close();
    ASSERT(iec.mountDisk(8, g64Path));
    iec.writePort(0x08); // ATN
    iec.tick(3000);
    std::remove(g64Path);
}
