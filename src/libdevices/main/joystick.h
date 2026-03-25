#pragma once

#include "iport_device.h"

/**
 * Standard 5-bit active-low joystick for VIC-20 and C64.
 * Mapping (active-low, 0 = pressed, 1 = idle):
 *   Bit 0: Up
 *   Bit 1: Down
 *   Bit 2: Left
 *   Bit 3: Right
 *   Bit 4: Fire
 *   Bits 5-7: Typically read as 1 on the VIC-20.
 */
class Joystick : public IPortDevice {
public:
    Joystick();
    virtual ~Joystick() = default;

    // IPortDevice interface
    uint8_t readPort() override;
    void writePort(uint8_t val) override;
    void setDdr(uint8_t ddr) override;

    /**
     * Sets the raw joystick state.
     * 1 = idle, 0 = pressed.
     */
    void setState(uint8_t bits);

private:
    uint8_t m_state; // Bits 0-4 are active-low
    uint8_t m_ddr;
};
