#pragma once

#include "libdevices/main/io_handler.h"
#include "libdevices/main/iport_device.h"
#include "libdevices/main/isignal_line.h"
#include <string>

/**
 * MOS 6520 / 6820 Peripheral Interface Adapter (PIA).
 *
 * Two independent 8-bit I/O ports (A and B) each with a data-direction
 * register, a pair of interrupt/control lines (Cx1 input, Cx2 in/out),
 * and a dedicated IRQ output.
 *
 * Register map — two address bits (RS[1:0]) select one of four locations:
 *   RS=00  ORA  (CRA bit 2 = 1) / DDRA (CRA bit 2 = 0)
 *   RS=01  CRA
 *   RS=10  ORB  (CRB bit 2 = 1) / DDRB (CRB bit 2 = 0)
 *   RS=11  CRB
 *
 * Control register (CRA / CRB) bit layout:
 *   7   IRQ1 flag   – set by Cx1 active edge; cleared by reading ORx  [read-only]
 *   6   IRQ2 flag   – set by Cx2 active edge (input mode only);
 *                     cleared by reading ORx unless independent mode  [read-only]
 *   5   Cx2 dir     0 = input, 1 = output
 *   4   Cx2 ctrl B
 *   3   Cx2 ctrl A  (input: 0=falling edge / 1=rising edge;
 *                    output bit4=0: 0=handshake / 1=pulse;
 *                    output bit4=1: 0=manual low / 1=manual high)
 *   2   DDR/OR sel  0 = DDR accessible at RS=x0, 1 = OR accessible
 *   1   Cx1 IRQ en  1 = active Cx1 edge sets IRQ1 flag and asserts IRQx
 *   0   Cx1 edge    0 = falling, 1 = rising
 *
 * CA2 output is triggered by an ORA read.
 * CB2 output is triggered by an ORB write.
 *
 * IRQx is asserted when (IRQ1 && Cx1_IRQ_EN) || (IRQ2 && Cx2_is_input).
 */
class PIA6520 : public IOHandler {
public:
    PIA6520() : m_name("6520"), m_baseAddr(0) { reset(); }
    PIA6520(const std::string& name, uint32_t baseAddr);
    ~PIA6520() override = default;

    void setName(const std::string& name) { m_name = name; }
    void setBaseAddr(uint32_t addr)        { m_baseAddr = addr; }

    // IOHandler interface
    const char* name()     const override { return m_name.c_str(); }
    uint32_t    baseAddr() const override { return m_baseAddr; }
    uint32_t    addrMask() const override { return 0x0003; }

    bool ioRead (IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t  val) override;
    void reset()  override;
    void tick(uint64_t cycles) override;

    // Peripheral attachment
    void setPortADevice(IPortDevice* dev) { m_portADevice = dev; }
    void setPortBDevice(IPortDevice* dev) { m_portBDevice = dev; }
    void setIrqALine(ISignalLine* line)   { m_irqaLine = line; }
    void setIrqBLine(ISignalLine* line)   { m_irqbLine = line; }
    void setCA1Line(ISignalLine* line)    { m_ca1Line  = line; }
    void setCA2Line(ISignalLine* line)    { m_ca2Line  = line; }
    void setCB1Line(ISignalLine* line)    { m_cb1Line  = line; }
    void setCB2Line(ISignalLine* line)    { m_cb2Line  = line; }

    // Snapshot — save / restore complete internal state
    struct Snapshot {
        uint8_t ora, ddra, cra;
        uint8_t orb, ddrb, crb;
        bool    ca1Prev, ca2Prev, cb1Prev, cb2Prev;
    };
    Snapshot    getSnapshot()                  const;
    void        restoreSnapshot(const Snapshot& s);

private:
    void updateIrqA();
    void updateIrqB();
    void driveCA2(bool level);
    void driveCB2(bool level);

    std::string  m_name;
    uint32_t     m_baseAddr;

    uint8_t  m_ora  = 0;
    uint8_t  m_ddra = 0;
    uint8_t  m_cra  = 0;   // bits 7,6 are read-only IRQ flags
    uint8_t  m_orb  = 0;
    uint8_t  m_ddrb = 0;
    uint8_t  m_crb  = 0;

    IPortDevice* m_portADevice = nullptr;
    IPortDevice* m_portBDevice = nullptr;
    ISignalLine* m_irqaLine    = nullptr;
    ISignalLine* m_irqbLine    = nullptr;
    ISignalLine* m_ca1Line     = nullptr;
    ISignalLine* m_ca2Line     = nullptr;
    ISignalLine* m_cb1Line     = nullptr;
    ISignalLine* m_cb2Line     = nullptr;

    bool m_ca1Prev = true;
    bool m_ca2Prev = true;
    bool m_cb1Prev = true;
    bool m_cb2Prev = true;
};
