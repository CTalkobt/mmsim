#pragma once

#include "libdevices/main/io_handler.h"
#include <vector>
#include <cstdint>

/**
 * MOS 6545 / Motorola 6845 Cathode Ray Tube Controller (CRTC).
 */
class CRTC6545 : public IOHandler {
public:
    CRTC6545();
    virtual ~CRTC6545() = default;

    // IOHandler implementation
    const char* name() const override { return "CRTC6545"; }
    uint32_t baseAddr() const override { return 0xE880; }
    uint32_t addrMask() const override { return 0x0001; } // Responds to $E880 and $E881

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

    // Internal register access for the video subsystem
    uint8_t getReg(uint8_t idx) const { return (idx < 18) ? m_regs[idx] : 0; }
    
    // Status bits
    uint16_t getMA() const { return m_ma; }      // Memory Address
    uint8_t  getRA() const { return m_ra; }      // Row Address (raster line within char)
    bool     getHSync() const { return m_hSync; }
    bool     getVSync() const { return m_vSync; }
    bool     getDispEnable() const { return m_dispEnable; }

private:
    uint8_t m_addressReg = 0;
    uint8_t m_regs[18];
    uint8_t m_statusReg = 0;

    // Internal counters
    uint8_t m_hCount = 0;       // Horizontal character counter (0 to R0)
    uint8_t m_vCount = 0;       // Vertical character row counter (0 to R4)
    uint8_t m_ra = 0;           // Row Address (raster line within char, 0 to R9)
    uint16_t m_ma = 0;          // Current memory address
    uint16_t m_maStart = 0;     // Memory address at start of current row

    bool m_hSync = false;
    bool m_vSync = false;
    bool m_dispEnable = false;

    void updateCounters();
};
