#pragma once

#include "libdevices/main/io_handler.h"
#include "libdevices/main/iport_device.h"
#include "libdevices/main/isignal_line.h"
#include <string>
#include <functional>

/**
 * MOS 6522 Versatile Interface Adapter (VIA).
 * Used in VIC-20, C64 (as 6526 CIA successor/relative), and many other systems.
 */
class VIA6522 : public IOHandler {
public:
    VIA6522() : m_name("6522"), m_baseAddr(0) { reset(); }
    VIA6522(const std::string& name, uint32_t baseAddr);
    virtual ~VIA6522() = default;

    // Configuration
    void setName(const std::string& name) { m_name = name; }
    void setBaseAddr(uint32_t addr) { m_baseAddr = addr; }

    // IOHandler interface
    const char* name() const override { return m_name.c_str(); }
    uint32_t baseAddr() const override { return m_baseAddr; }
    uint32_t addrMask() const override { return 0x000F; } // 16 registers

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

    // Peripheral attachment
    void setPortADevice(IPortDevice* device) { m_portADevice = device; }
    void setPortBDevice(IPortDevice* device) { m_portBDevice = device; }
    void setIrqLine(ISignalLine* line) { m_irqLine = line; }
    void setCA1Line(ISignalLine* line)  { m_ca1Line = line; }
    void setCA2Line(ISignalLine* line)  { m_ca2Line = line; }
    void setCB1Line(ISignalLine* line)  { m_cb1Line = line; }
    void setCB2Line(ISignalLine* line)  { m_cb2Line = line; }

    void setPortAWriteCallback(std::function<void(uint8_t)> cb) { m_portAWriteCb = cb; }
    void setPortBWriteCallback(std::function<void(uint8_t)> cb) { m_portBWriteCb = cb; }

    // Register constants
    enum Reg {
        ORB   = 0x0,
        ORA   = 0x1,
        DDRB  = 0x2,
        DDRA  = 0x3,
        T1CL  = 0x4,
        T1CH  = 0x5,
        T1LL  = 0x6,
        T1LH  = 0x7,
        T2CL  = 0x8,
        T2CH  = 0x9,
        SR    = 0xA,
        ACR   = 0xB,
        PCR   = 0xC,
        IFR   = 0xD,
        IER   = 0xE,
        IORA2 = 0xF  // ORA without handshake
    };

private:
    void updateIrq();
    void driveCA2(bool level);
    void driveCB2(bool level);

    std::string m_name;
    uint32_t    m_baseAddr;

    uint8_t m_regs[16];

    // Latches for Timer 1
    uint16_t m_t1Latch = 0;
    uint16_t m_t1Counter = 0;
    bool     m_t1Active = false;

    // Timer 2
    uint16_t m_t2Counter = 0;
    bool     m_t2Active = false;

    IPortDevice* m_portADevice = nullptr;
    IPortDevice* m_portBDevice = nullptr;
    ISignalLine* m_irqLine  = nullptr;
    ISignalLine* m_ca1Line  = nullptr;  // CA1 input
    ISignalLine* m_ca2Line  = nullptr;  // CA2 input/output (depends on PCR)
    ISignalLine* m_cb1Line  = nullptr;  // CB1 input
    ISignalLine* m_cb2Line  = nullptr;  // CB2 input/output (depends on PCR)

    std::function<void(uint8_t)> m_portAWriteCb;
    std::function<void(uint8_t)> m_portBWriteCb;

    bool m_ca1Prev = true;  // last sampled CA1 level
    bool m_ca2Prev = true;
    bool m_cb1Prev = true;
    bool m_cb2Prev = true;

    bool m_lastIrq = false;
};
