#include "test_harness.h"
#include "libdevices/main/joystick.h"

TEST_CASE(joystick_basic) {
    Joystick joy;
    
    // Default state: all bits high (idle)
    ASSERT(joy.readPort() == 0xFF);
    
    // Press Up (Bit 0 = 0)
    joy.setState(0x1E);
    ASSERT(joy.readPort() == 0xFE);
    
    // Press Fire (Bit 4 = 0)
    joy.setState(0x0F);
    ASSERT(joy.readPort() == 0xEF);
    
    // Press Up and Fire
    joy.setState(0x0E);
    ASSERT(joy.readPort() == 0xEE);
    
    // Release all
    joy.setState(0x1F);
    ASSERT(joy.readPort() == 0xFF);
}
