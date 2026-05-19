#include "vic4.h"
#include <cstring>

VIC4::VIC4()
    : VIC3("VIC-IV", 0xD000)
{
    m_colorRamExt.resize(32768, 0); // 32 KB internal color RAM
    reset();
}

void VIC4::reset() {
    VIC3::reset();
    std::memset(m_extRegs, 0, sizeof(m_extRegs));

    // Default VIC-IV extended registers (indexed as $D0xx - $D040)
    // $D060-$D063: SCRNPTR — screen RAM base (28-bit), default $0000
    m_extRegs[0x20] = 0x00; // SCRNPTRLSB
    m_extRegs[0x21] = 0x00; // SCRNPTRMSB
    m_extRegs[0x22] = 0x00; // SCRNPTRBNK
    m_extRegs[0x23] = 0x00; // SCRNPTRMB (bits 27-24)

    // $D064-$D065: COLPTR — colour RAM base, default $0000
    m_extRegs[0x24] = 0x00; // COLPTRLSB
    m_extRegs[0x25] = 0x00; // COLPTRMSB

    // $D068-$D06A: CHARPTR — character set base (24-bit), default $0000
    m_extRegs[0x28] = 0x00; // CHARPTRLSB
    m_extRegs[0x29] = 0x00; // CHARPTRMSB
    m_extRegs[0x2A] = 0x00; // CHARPTRBNK
}

void VIC4::tick(uint64_t cycles) {
    VIC3::tick(cycles);
}

bool VIC4::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;

    uint16_t offset = addr & 0x03FF;

    // $D048-$D07F: VIC-IV specific extended registers
    if (offset >= 0x0048 && offset <= 0x007F) {
        if (isLocked()) { *val = 0xFF; return true; }
        *val = m_extRegs[offset - 0x0040];
        return true;
    }

    // Everything else ($D000-$D047, $D080-$D3FF) handled by VIC3
    return VIC3::ioRead(bus, addr, val);
}

bool VIC4::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;

    uint16_t offset = addr & 0x03FF;

    // $D048-$D07F: VIC-IV specific extended registers
    if (offset >= 0x0048 && offset <= 0x007F) {
        if (isLocked()) return true;
        m_extRegs[offset - 0x0040] = val;
        return true;
    }

    // Everything else handled by VIC3
    return VIC3::ioWrite(bus, addr, val);
}

uint32_t VIC4::getScreenBase() const {
    if (isLocked()) return VIC2::screenBase();
    // $D060-$D063: SCRNPTR (28-bit physical address)
    // m_extRegs indexed as $D0xx - $D040
    return ((uint32_t)(m_extRegs[0x23] & 0x0F) << 24) |
           ((uint32_t)m_extRegs[0x22] << 16) |
           ((uint32_t)m_extRegs[0x21] << 8) |
           (uint32_t)m_extRegs[0x20];
}

uint32_t VIC4::getCharBase() const {
    if (isLocked()) return VIC2::charBitmapBase();
    // $D068-$D06A: CHARPTR (24-bit physical address)
    return ((uint32_t)m_extRegs[0x2A] << 16) |
           ((uint32_t)m_extRegs[0x29] << 8) |
           (uint32_t)m_extRegs[0x28];
}

void VIC4::renderFrame(uint32_t* buffer) {
    if (isLocked()) {
        VIC2::renderFrame(buffer);
        return;
    }

    // MEGA65 unlocked mode: use VIC-III rendering as baseline
    // VIC-IV specific modes (FCM, NCM, etc.) will override this later
    VIC3::renderFrame(buffer);
}
