#include "vic6560.h"
#include <cstring>

VIC6560::VIC6560(const std::string& name, uint32_t baseAddr)
    : m_name(name), m_baseAddr(baseAddr)
{
    reset();
}

void VIC6560::reset() {
    std::memset(m_regs, 0, sizeof(m_regs));
    m_rasterCounter = 0;
}

bool VIC6560::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    (void)bus;
    if ((addr & ~addrMask()) != m_baseAddr) return false;
    *val = m_regs[addr & 0xF];
    return true;
}

bool VIC6560::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    (void)bus;
    if ((addr & ~addrMask()) != m_baseAddr) return false;
    m_regs[addr & 0xF] = val;
    return true;
}

void VIC6560::tick(uint64_t cycles) {
    m_rasterCounter += cycles;
    // Basic raster simulation (approx 65 cycles per line)
    uint8_t line = (m_rasterCounter / 65) % 262;
    m_regs[0x04] = line; // Raster counter register
}
