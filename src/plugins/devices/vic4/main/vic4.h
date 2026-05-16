#pragma once

#include "plugins/devices/vic2/main/vic2.h"
#include <vector>

/**
 * VIC-IV Video Interface Chip for MEGA65.
 * 
 * Extends the VIC-II with:
 * - MEGA65 personality unlock ($D02F KEY).
 * - Extended registers ($D040–$D07F).
 * - 256-color palette ($D100–$D3FF).
 * - 80-column and Full Color Mode (FCM).
 * - 28-bit physical addressing for all video data.
 * - 32 KB internal color RAM.
 */
class VIC4 : public VIC2 {
public:
    VIC4();
    ~VIC4() override = default;

    // IOHandler overrides
    const char* name() const override { return "VIC-IV"; }
    uint32_t addrMask() const override { return 0x03FF; } // 1 KB window ($D000-$D3FF)

    bool ioRead (IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t  val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

    // IVideoOutput overrides
    void renderFrame(uint32_t* buffer) override;

    // Personality management
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }

private:
    // Extended register accessors
    uint32_t getScreenBase() const;
    uint32_t getCharBase() const;
    uint32_t getColorRamBase() const;

    bool m_locked = true; // Locked to VIC-II mode by default
    
    // Extended registers $D040-$D07F (64 bytes)
    uint8_t m_extRegs[64];

    // Palette: 256 entries x 3 bytes (R, G, B)
    // Stored in three 256-byte arrays corresponding to $D100, $D200, $D300
    uint8_t m_paletteR[256];
    uint8_t m_paletteG[256];
    uint8_t m_paletteB[256];

    // Internal color RAM (32 KB)
    // Default base is $FF80000.
    std::vector<uint8_t> m_colorRamExt;

    uint32_t getPaletteRGBA(uint8_t index) const;
};
