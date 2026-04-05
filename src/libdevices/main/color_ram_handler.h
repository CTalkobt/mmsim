#pragma once

#include "io_handler.h"
#include <cstdint>
#include <string>

/**
 * Generic 4-bit color RAM handler.
 * Reads return the low nibble OR'd with 0xF0 (upper nibble floats high).
 * Writes mask to the low nibble.
 * Used by VIC-20 ($9400) and C64 ($D800).
 */
class ColorRamHandler : public IOHandler {
public:
    ColorRamHandler(uint32_t base, uint32_t size, uint8_t* ram)
        : m_base(base), m_size(size), m_ram(ram) {}

    const char* name()     const override { return "ColorRAM"; }
    uint32_t    baseAddr() const override { return m_base; }
    uint32_t    addrMask() const override { return m_size - 1; }

    bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
        if (addr < m_base || addr >= m_base + m_size) return false;
        *val = m_ram[addr - m_base] | 0xF0;
        return true;
    }
    bool ioWrite(IBus*, uint32_t addr, uint8_t val) override {
        if (addr < m_base || addr >= m_base + m_size) return false;
        m_ram[addr - m_base] = val & 0x0F;
        return true;
    }
    void reset() override {}
    void tick(uint64_t) override {}

    uint8_t* buffer() const { return m_ram; }

private:
    uint32_t m_base, m_size;
    uint8_t* m_ram;
};
