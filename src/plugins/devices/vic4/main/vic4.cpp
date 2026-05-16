#include "vic4.h"
#include <cstring>
#include <algorithm>

VIC4::VIC4()
    : VIC2("VIC-IV", 0xD000)
{
    m_colorRamExt.resize(32768, 0); // 32 KB internal color RAM
    reset();
}

void VIC4::reset() {
    VIC2::reset();
    m_locked = true;
    std::memset(m_extRegs, 0, sizeof(m_extRegs));
    
    // Default palette (C64 compatible)
    for (int i = 0; i < 256; ++i) {
        m_paletteR[i] = 0;
        m_paletteG[i] = 0;
        m_paletteB[i] = 0;
    }
    
    // Initialize first 16 colors with C64 palette
    static const uint8_t c64R[] = { 0x00, 0xFF, 0x68, 0x70, 0x6F, 0x58, 0x35, 0xB8, 0x6F, 0x43, 0x9A, 0x44, 0x6C, 0x9A, 0x6C, 0x95 };
    static const uint8_t c64G[] = { 0x00, 0xFF, 0x37, 0xA4, 0x3D, 0x8D, 0x28, 0xC7, 0x4F, 0x39, 0x67, 0x44, 0x6C, 0xD2, 0x5E, 0x95 };
    static const uint8_t c64B[] = { 0x00, 0xFF, 0x2B, 0xB2, 0x86, 0x43, 0x79, 0x6F, 0x25, 0x00, 0x59, 0x44, 0x6C, 0x84, 0xB5, 0x95 };

    for (int i = 0; i < 16; ++i) {
        m_paletteR[i] = c64R[i];
        m_paletteG[i] = c64G[i];
        m_paletteB[i] = c64B[i];
    }

    // Default VIC-IV registers
    m_extRegs[0x0C] = 0x00; // Screen RAM base $0000000
    m_extRegs[0x0D] = 0x00;
    m_extRegs[0x0E] = 0x00;
    m_extRegs[0x0F] = 0x00;

    m_extRegs[0x10] = 0x00; // Char base $0000000
    m_extRegs[0x11] = 0x00;
    m_extRegs[0x12] = 0x00;
    m_extRegs[0x13] = 0x00;

    m_extRegs[0x18] = 0x00; // Color RAM base $FF80000 (effectively default)
    m_extRegs[0x19] = 0x80;
    m_extRegs[0x1A] = 0xF8;
    m_extRegs[0x1B] = 0x0F;
}

void VIC4::tick(uint64_t cycles) {
    // Forward to VIC2 for now (handles raster and IRQ)
    VIC2::tick(cycles);
}

bool VIC4::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;
    
    uint16_t offset = addr & 0x03FF;

    // $D000 - $D02E: Standard VIC-II registers
    if (offset <= 0x002E) {
        return VIC2::ioRead(bus, addr, val);
    }

    // $D02F: KEY register (handled by KeyRegister, but we mirror if needed)
    if (offset == 0x002F) {
        *val = 0xFF; // Handled elsewhere
        return false;
    }

    // $D030 - $D03F: VIC-III registers
    if (offset >= 0x0030 && offset <= 0x003F) {
        if (m_locked) {
            *val = 0xFF;
            return true;
        }
        *val = m_regs[offset & 0x3F];
        return true;
    }

    // $D040 - $D07F: VIC-IV Extended registers
    if (offset >= 0x0040 && offset <= 0x007F) {
        if (m_locked) {
            *val = 0xFF;
            return true;
        }
        *val = m_extRegs[offset - 0x0040];
        return true;
    }

    // $D100 - $D1FF: Palette Red
    if (offset >= 0x0100 && offset <= 0x01FF) {
        if (m_locked) {
            *val = 0xFF;
            return true;
        }
        *val = m_paletteR[offset & 0xFF];
        return true;
    }

    // $D200 - $D2FF: Palette Green
    if (offset >= 0x0200 && offset <= 0x02FF) {
        if (m_locked) {
            *val = 0xFF;
            return true;
        }
        *val = m_paletteG[offset & 0xFF];
        return true;
    }

    // $D300 - $D3FF: Palette Blue
    if (offset >= 0x0300 && offset <= 0x03FF) {
        if (m_locked) {
            *val = 0xFF;
            return true;
        }
        *val = m_paletteB[offset & 0xFF];
        return true;
    }

    return false;
}

bool VIC4::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;

    uint16_t offset = addr & 0x03FF;

    // $D000 - $D02E: Standard VIC-II registers
    if (offset <= 0x002E) {
        return VIC2::ioWrite(bus, addr, val);
    }

    // $D02F: KEY register
    if (offset == 0x002F) {
        return false; // Handled by KeyRegister
    }

    // $D030 - $D03F: VIC-III registers
    if (offset >= 0x0030 && offset <= 0x003F) {
        if (m_locked) return true;
        m_regs[offset & 0x3F] = val;
        return true;
    }

    // $D040 - $D07F: VIC-IV Extended registers
    if (offset >= 0x0040 && offset <= 0x007F) {
        if (m_locked) return true;
        m_extRegs[offset - 0x0040] = val;
        return true;
    }

    // $D100 - $D1FF: Palette Red
    if (offset >= 0x0100 && offset <= 0x01FF) {
        if (m_locked) return true;
        m_paletteR[offset & 0xFF] = val;
        return true;
    }

    // $D200 - $D2FF: Palette Green
    if (offset >= 0x0200 && offset <= 0x02FF) {
        if (m_locked) return true;
        m_paletteG[offset & 0xFF] = val;
        return true;
    }

    // $D300 - $D3FF: Palette Blue
    if (offset >= 0x0300 && offset <= 0x03FF) {
        if (m_locked) return true;
        m_paletteB[offset & 0xFF] = val;
        return true;
    }

    return false;
}

uint32_t VIC4::getPaletteRGBA(uint8_t index) const {
    return 0xFF000000 | (m_paletteB[index] << 16) | (m_paletteG[index] << 8) | m_paletteR[index];
}

uint32_t VIC4::getScreenBase() const {
    if (m_locked) return VIC2::screenBase();
    // $D04C-$D04F: screen RAM base address (32-bit)
    return (m_extRegs[0x0F] << 24) | (m_extRegs[0x0E] << 16) | (m_extRegs[0x0D] << 8) | m_extRegs[0x0C];
}

uint32_t VIC4::getCharBase() const {
    if (m_locked) return VIC2::charBitmapBase();
    // $D050-$D053: character set base address (32-bit)
    return (m_extRegs[0x13] << 24) | (m_extRegs[0x12] << 16) | (m_extRegs[0x11] << 8) | m_extRegs[0x10];
}

void VIC4::renderFrame(uint32_t* buffer) {
    if (m_locked) {
        VIC2::renderFrame(buffer);
        return;
    }

    // Implement MEGA65 rendering logic...
    // For now, just clear to border color using extended palette
    uint32_t borderCol = getPaletteRGBA(m_regs[0x20] & 0xFF);
    for (int i = 0; i < FRAME_W * FRAME_H; ++i) buffer[i] = borderCol;
    
    // TODO: Full raster pipeline for VIC-IV
}
