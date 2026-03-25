#include "ibus.h"

uint16_t IBus::read16(uint32_t addr) {
    uint8_t lo = read8(addr);
    uint8_t hi = read8(addr + 1);
    if (config().littleEndian) {
        return (uint16_t)lo | ((uint16_t)hi << 8);
    } else {
        return (uint16_t)hi | ((uint16_t)lo << 8);
    }
}

void IBus::write16(uint32_t addr, uint16_t val) {
    uint8_t lo = val & 0xFF;
    uint8_t hi = (val >> 8) & 0xFF;
    if (config().littleEndian) {
        write8(addr, lo);
        write8(addr + 1, hi);
    } else {
        write8(addr, hi);
        write8(addr + 1, lo);
    }
}

uint32_t IBus::read32(uint32_t addr) {
    uint16_t lo = read16(addr);
    uint16_t hi = read16(addr + 2);
    if (config().littleEndian) {
        return (uint32_t)lo | ((uint32_t)hi << 16);
    } else {
        return (uint32_t)hi | ((uint32_t)lo << 16);
    }
}

void IBus::write32(uint32_t addr, uint32_t val) {
    uint16_t lo = val & 0xFFFF;
    uint16_t hi = (val >> 16) & 0xFFFF;
    if (config().littleEndian) {
        write16(addr, lo);
        write16(addr + 2, hi);
    } else {
        write16(addr, hi);
        write16(addr + 2, lo);
    }
}
