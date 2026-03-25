#include "via6522.h"
#include <cstring>

VIA6522::VIA6522(const std::string& name, uint32_t baseAddr)
    : m_name(name), m_baseAddr(baseAddr)
{
    reset();
}

void VIA6522::reset() {
    std::memset(m_regs, 0, sizeof(m_regs));
    m_t1Latch = 0xFFFF;
    m_t1Counter = 0;
    m_t1Active = false;
    m_t2Counter = 0;
    m_t2Active = false;
    if (m_irqLine) m_irqLine->set(false);
}

bool VIA6522::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    (void)bus;
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    uint8_t reg = addr & 0xF;
    switch (reg) {
        case ORA:
        case IORA2:
            *val = m_regs[ORA];
            if (m_portADevice) {
                uint8_t pins = m_portADevice->readPort();
                *val = (m_regs[ORA] & m_regs[DDRA]) | (pins & ~m_regs[DDRA]);
            }
            // Handshake normally clears CA1 if ORA is read, but IORA2 doesn't.
            if (reg == ORA) m_regs[IFR] &= ~0x02; // Placeholder for CA1
            break;

        case ORB:
            *val = m_regs[ORB];
            if (m_portBDevice) {
                uint8_t pins = m_portBDevice->readPort();
                *val = (m_regs[ORB] & m_regs[DDRB]) | (pins & ~m_regs[DDRB]);
            }
            m_regs[IFR] &= ~0x10; // Placeholder for CB1
            break;

        case T1CL:
            *val = m_t1Counter & 0xFF;
            m_regs[IFR] &= ~0x40; // Clear T1 interrupt
            updateIrq();
            break;

        case T1CH:
            *val = (m_t1Counter >> 8) & 0xFF;
            break;

        case T2CL:
            *val = m_t2Counter & 0xFF;
            m_regs[IFR] &= ~0x20; // Clear T2 interrupt
            updateIrq();
            break;

        case T2CH:
            *val = (m_t2Counter >> 8) & 0xFF;
            break;

        case IFR:
            *val = m_regs[IFR];
            break;

        default:
            *val = m_regs[reg];
            break;
    }
    return true;
}

bool VIA6522::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    (void)bus;
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    uint8_t reg = addr & 0xF;
    switch (reg) {
        case ORA:
        case IORA2:
            m_regs[ORA] = val;
            if (m_portADevice) m_portADevice->writePort(val);
            if (reg == ORA) m_regs[IFR] &= ~0x02; // Placeholder for CA1
            break;

        case ORB:
            m_regs[ORB] = val;
            if (m_portBDevice) m_portBDevice->writePort(val);
            m_regs[IFR] &= ~0x10; // Placeholder for CB1
            break;

        case DDRA:
            m_regs[DDRA] = val;
            if (m_portADevice) m_portADevice->setDdr(val);
            break;

        case DDRB:
            m_regs[DDRB] = val;
            if (m_portBDevice) m_portBDevice->setDdr(val);
            break;

        case T1CL:
        case T1LL:
            m_t1Latch = (m_t1Latch & 0xFF00) | val;
            break;

        case T1CH:
            m_t1Latch = (m_t1Latch & 0x00FF) | (val << 8);
            m_t1Counter = m_t1Latch;
            m_t1Active = true;
            m_regs[IFR] &= ~0x40; // Clear T1 interrupt
            updateIrq();
            break;

        case T1LH:
            m_t1Latch = (m_t1Latch & 0x00FF) | (val << 8);
            m_regs[IFR] &= ~0x40; // Clear T1 interrupt
            updateIrq();
            break;

        case T2CL:
            m_regs[T2CL] = val; // Actually latches into T2CL latch
            break;

        case T2CH:
            m_t2Counter = (val << 8) | m_regs[T2CL];
            m_t2Active = true;
            m_regs[IFR] &= ~0x20; // Clear T2 interrupt
            updateIrq();
            break;

        case IFR:
            // Bits 0-6 are cleared by writing a 1 to them
            m_regs[IFR] &= ~(val & 0x7F);
            updateIrq();
            break;

        case IER:
            if (val & 0x80) {
                m_regs[IER] |= (val & 0x7F);
            } else {
                m_regs[IER] &= ~(val & 0x7F);
            }
            updateIrq();
            break;

        default:
            m_regs[reg] = val;
            break;
    }
    return true;
}

void VIA6522::tick(uint64_t cycles) {
    // Tick Timer 1
    if (m_t1Active) {
        uint16_t prev = m_t1Counter;
        m_t1Counter -= (uint16_t)cycles;
        if (prev < (uint16_t)cycles) { // Underflow
            m_regs[IFR] |= 0x40;
            updateIrq();
            
            // Continuous mode?
            if (m_regs[ACR] & 0x40) {
                m_t1Counter = m_t1Latch;
            } else {
                m_t1Active = false;
            }
        }
    }

    // Tick Timer 2 (Simplified: only one-shot interval mode)
    if (m_t2Active) {
        uint16_t prev = m_t2Counter;
        m_t2Counter -= (uint16_t)cycles;
        if (prev < (uint16_t)cycles) { // Underflow
            m_regs[IFR] |= 0x20;
            updateIrq();
            m_t2Active = false;
        }
    }
}

void VIA6522::updateIrq() {
    bool irq = (m_regs[IFR] & m_regs[IER] & 0x7F) != 0;
    if (irq) {
        m_regs[IFR] |= 0x80;
    } else {
        m_regs[IFR] &= ~0x80;
    }
    if (m_irqLine) m_irqLine->set(irq);
}
