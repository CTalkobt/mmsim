#pragma once

#include "libdevices/main/io_handler.h"
#include <string>

/**
 * MOS 6560/6561 Video Interface Chip (VIC-I).
 * Used in the VIC-20.
 */
class VIC6560 : public IOHandler {
public:
    VIC6560() : m_name("VIC-I"), m_baseAddr(0x9000) { reset(); }
    VIC6560(const std::string& name, uint32_t baseAddr);
    virtual ~VIC6560() = default;

    // Configuration
    void setName(const std::string& name) { m_name = name; }
    void setBaseAddr(uint32_t addr) { m_baseAddr = addr; }

    std::string name() const override { return m_name; }
    uint32_t baseAddr() const override { return m_baseAddr; }
    uint32_t addrMask() const override { return 0x000F; } // 16 registers

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

private:
    std::string m_name;
    uint32_t    m_baseAddr;
    uint8_t     m_regs[16];
    uint64_t    m_rasterCounter = 0;
};
