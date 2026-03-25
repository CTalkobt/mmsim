#pragma once

#include "libdevices/main/io_handler.h"
#include "libdevices/main/ivideo_output.h"
#include "libdevices/main/isignal_line.h"
#include <string>

/**
 * MOS 6560/6561 Video Interface Chip (VIC-I).
 * Used in the VIC-20.
 */
class VIC6560 : public IOHandler, public IVideoOutput {
public:
    VIC6560() : m_name("VIC-I"), m_baseAddr(0x9000) { reset(); }
    VIC6560(const std::string& name, uint32_t baseAddr);
    virtual ~VIC6560() = default;

    // Configuration
    void setName(const std::string& name) { m_name = name; }
    void setBaseAddr(uint32_t addr) { m_baseAddr = addr; }
    void setBus(IBus* bus) { m_bus = bus; }
    void setIrqLine(ISignalLine* line) { m_irqLine = line; }
    void setColorRam(const uint8_t* colorRam) { m_colorRam = colorRam; }

    // IOHandler interface
    const char* name() const override { return m_name.c_str(); }
    uint32_t baseAddr() const override { return m_baseAddr; }
    uint32_t addrMask() const override { return 0x000F; } // 16 registers

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

    // IVideoOutput interface
    VideoDimensions getDimensions() const override;
    void renderFrame(uint32_t* buffer) override;

private:
    uint32_t getVicColor(uint8_t index);

    std::string m_name;
    uint32_t    m_baseAddr;
    uint8_t     m_regs[16];
    uint64_t    m_rasterCounter = 0;
    
    ISignalLine* m_irqLine = nullptr;
    const uint8_t* m_colorRam = nullptr; // 4-bit nibbles, usually at $9400 or $9600
    IBus* m_bus = nullptr;
};
