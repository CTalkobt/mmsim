#include "test_harness.h"
#include "virtual_iec.h"

TEST_CASE(virtual_iec_handshake) {
    VirtualIECBus iec(8);
    
    // Initial state: all lines released (False/0)
    iec.writePort(0x00);
    ASSERT(iec.readPort() == 0x00);
    
    // Host pulls ATN (bit 3 = 1)
    iec.writePort(0x08);
    
    // Tick to detect ATN
    iec.tick(3000); 
    
    // Device should respond by pulling DATA (bit 7 in readPort)
    uint8_t val = iec.readPort();
    ASSERT(val & 0x80);
    
    // Host sends command: LISTEN 8 (0x28)
    // Bits: 0 0 0 1 0 1 0 0 (LSB first)
    uint8_t cmd = 0x28;
    for (int i = 0; i < 8; ++i) {
        bool bit = (cmd >> i) & 1;
        
        // Host releases CLK (bit 4 = 0)
        iec.writePort(0x08 | (bit ? 0x20 : 0x00));
        iec.tick(100);
        
        // Host pulls CLK (bit 4 = 1)
        iec.writePort(0x18 | (bit ? 0x20 : 0x00));
        iec.tick(100);
    }
    
    // After 8 bits, device pulls DATA to acknowledge byte
    val = iec.readPort();
    ASSERT(val & 0x80);
}
