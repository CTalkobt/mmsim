#pragma once

#include "io_handler.h"
#include <cstdint>
#include <string>

/**
 * Open-bus handler: covers an unmapped address range.
 * Reads return 0xFF; writes are silently consumed.
 * Used for VIC-20 expansion RAM blocks that are not fitted.
 */
class OpenBusHandler : public IOHandler {
public:
    OpenBusHandler(uint32_t base, uint32_t size, const std::string& name = "OpenBus")
        : m_name(name), m_base(base), m_size(size) {}

    const char* name()     const override { return m_name.c_str(); }
    uint32_t    baseAddr() const override { return m_base; }
    uint32_t    addrMask() const override { return m_size - 1; }

    bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
        if (addr < m_base || addr >= m_base + m_size) return false;
        *val = 0xFF;
        return true;
    }
    bool ioWrite(IBus*, uint32_t addr, uint8_t) override {
        return addr >= m_base && addr < m_base + m_size;
    }
    void reset() override {}
    void tick(uint64_t) override {}

private:
    std::string m_name;
    uint32_t    m_base, m_size;
};
