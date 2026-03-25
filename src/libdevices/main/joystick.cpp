#include "joystick.h"

Joystick::Joystick() : m_state(0x1F), m_ddr(0) {}

uint8_t Joystick::readPort() {
    // 0 = pressed, 1 = idle.
    // Unconnected pins (5-7) typically read as 1.
    // If the CPU sets the DDR to output (1), then the read value should reflect
    // the value set by the CPU (writePort). However, a joystick is purely input.
    // We'll follow the standard model: reflect the state, masked by the DDR
    // for output bits (though Joystick has none) and then OR with 0xE0 for
    // high bits 5-7.
    return (m_state & 0x1F) | 0xE0;
}

void Joystick::writePort(uint8_t val) {
    // Joystick is an input device; writes generally have no effect on the peripheral.
    (void)val;
}

void Joystick::setDdr(uint8_t ddr) {
    m_ddr = ddr;
}

void Joystick::setState(uint8_t bits) {
    m_state = bits & 0x1F;
}
