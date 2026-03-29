#include "pia6520.h"

PIA6520::PIA6520(const std::string& name, uint32_t baseAddr)
    : m_name(name), m_baseAddr(baseAddr)
{
    reset();
}

void PIA6520::reset() {
    m_ora = m_ddra = m_cra = 0;
    m_orb = m_ddrb = m_crb = 0;
    m_ca1Prev = m_ca2Prev = m_cb1Prev = m_cb2Prev = true;
    if (m_portADevice) m_portADevice->setDdr(0);
    if (m_portBDevice) m_portBDevice->setDdr(0);
    if (m_irqaLine) m_irqaLine->set(false);
    if (m_irqbLine) m_irqbLine->set(false);
    driveCA2(true);
    driveCB2(true);
}

// ---------------------------------------------------------------------------
// IRQ update helpers
// ---------------------------------------------------------------------------

// IRQx asserted when (IRQ1 && Cx1_IRQ_EN) || (IRQ2 && Cx2_is_input)
void PIA6520::updateIrqA() {
    bool irq = ((m_cra & 0x80) && (m_cra & 0x02)) ||   // CA1 flag + enable
               ((m_cra & 0x40) && !(m_cra & 0x20));    // CA2 flag + CA2 is input
    if (m_irqaLine) m_irqaLine->set(irq);
}

void PIA6520::updateIrqB() {
    bool irq = ((m_crb & 0x80) && (m_crb & 0x02)) ||
               ((m_crb & 0x40) && !(m_crb & 0x20));
    if (m_irqbLine) m_irqbLine->set(irq);
}

void PIA6520::driveCA2(bool level) {
    if (m_ca2Line) m_ca2Line->set(level);
}

void PIA6520::driveCB2(bool level) {
    if (m_cb2Line) m_cb2Line->set(level);
}

// ---------------------------------------------------------------------------
// IOHandler — reads
// ---------------------------------------------------------------------------

bool PIA6520::ioRead(IBus* /*bus*/, uint32_t addr, uint8_t* val) {
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    switch (addr & 0x03) {
        case 0x00:  // ORA (CRA bit 2 = 1) or DDRA (CRA bit 2 = 0)
            if (m_cra & 0x04) {
                if (m_portADevice) {
                    uint8_t pins = m_portADevice->readPort();
                    *val = (m_ora & m_ddra) | (pins & ~m_ddra);
                } else {
                    *val = m_ora;
                }
                // Clear IRQ1 flag unconditionally; clear IRQ2 unless independent mode
                m_cra &= ~0x80;
                if (!(m_cra & 0x10))  // not independent mode (CRA bit 4 = 0)
                    m_cra &= ~0x40;
                // CA2 output: handshake/pulse asserted on ORA read
                if (m_cra & 0x20) {   // CA2 is output
                    if (!(m_cra & 0x10)) {  // bit 4 = 0: strobe mode
                        if (m_cra & 0x08) { driveCA2(false); driveCA2(true); } // pulse
                        else              { driveCA2(false); }                  // handshake
                    }
                }
                updateIrqA();
            } else {
                *val = m_ddra;
            }
            break;

        case 0x01:  // CRA (bits 7,6 are live IRQ flags)
            *val = m_cra;
            break;

        case 0x02:  // ORB (CRB bit 2 = 1) or DDRB (CRB bit 2 = 0)
            if (m_crb & 0x04) {
                if (m_portBDevice) {
                    uint8_t pins = m_portBDevice->readPort();
                    *val = (m_orb & m_ddrb) | (pins & ~m_ddrb);
                } else {
                    *val = m_orb;
                }
                // Clear IRQ flags on ORB read (CB2 output triggered by write, not read)
                m_crb &= ~0x80;
                if (!(m_crb & 0x10))
                    m_crb &= ~0x40;
                updateIrqB();
            } else {
                *val = m_ddrb;
            }
            break;

        case 0x03:  // CRB
            *val = m_crb;
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// IOHandler — writes
// ---------------------------------------------------------------------------

bool PIA6520::ioWrite(IBus* /*bus*/, uint32_t addr, uint8_t val) {
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    switch (addr & 0x03) {
        case 0x00:  // ORA (CRA bit 2 = 1) or DDRA (CRA bit 2 = 0)
            if (m_cra & 0x04) {
                m_ora = val;
                if (m_portADevice) m_portADevice->writePort(val);
            } else {
                m_ddra = val;
                if (m_portADevice) m_portADevice->setDdr(val);
            }
            break;

        case 0x01:  // CRA — bits 7,6 are read-only; mask them out
            m_cra = (m_cra & 0xC0) | (val & 0x3F);
            // Apply CA2 manual output immediately when in manual mode (bits 5,4 = 1,1)
            if ((m_cra & 0x30) == 0x30)
                driveCA2((m_cra & 0x08) ? true : false);
            updateIrqA();
            break;

        case 0x02:  // ORB (CRB bit 2 = 1) or DDRB (CRB bit 2 = 0)
            if (m_crb & 0x04) {
                m_orb = val;
                if (m_portBDevice) m_portBDevice->writePort(val);
                // Clear IRQ flags on ORB write
                m_crb &= ~0x80;
                if (!(m_crb & 0x10))
                    m_crb &= ~0x40;
                // CB2 output: handshake/pulse asserted on ORB write
                if (m_crb & 0x20) {   // CB2 is output
                    if (!(m_crb & 0x10)) {  // strobe mode
                        if (m_crb & 0x08) { driveCB2(false); driveCB2(true); } // pulse
                        else              { driveCB2(false); }                  // handshake
                    }
                }
                updateIrqB();
            } else {
                m_ddrb = val;
                if (m_portBDevice) m_portBDevice->setDdr(val);
            }
            break;

        case 0x03:  // CRB
            m_crb = (m_crb & 0xC0) | (val & 0x3F);
            if ((m_crb & 0x30) == 0x30)
                driveCB2((m_crb & 0x08) ? true : false);
            updateIrqB();
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// tick — edge detection on Cx1 / Cx2 input lines
// ---------------------------------------------------------------------------

void PIA6520::tick(uint64_t /*cycles*/) {
    // CA1: always an input; active edge sets IRQ1 flag (bit 7 of CRA)
    if (m_ca1Line) {
        bool lvl = m_ca1Line->get();
        bool activeEdge = (m_cra & 0x01) ? (!m_ca1Prev && lvl)   // rising
                                          : (m_ca1Prev && !lvl);  // falling
        if (activeEdge) {
            m_cra |= 0x80;
            // In handshake output mode, active CA1 edge releases CA2
            if ((m_cra & 0x20) && !(m_cra & 0x10) && !(m_cra & 0x08))
                driveCA2(true);
            updateIrqA();
        }
        m_ca1Prev = lvl;
    }

    // CA2 as input: active edge sets IRQ2 flag (bit 6 of CRA)
    if (m_ca2Line && !(m_cra & 0x20)) {
        bool lvl = m_ca2Line->get();
        bool activeEdge = (m_cra & 0x08) ? (!m_ca2Prev && lvl)
                                          : (m_ca2Prev && !lvl);
        if (activeEdge) {
            m_cra |= 0x40;
            updateIrqA();
        }
        m_ca2Prev = lvl;
    }

    // CB1: always an input; active edge sets IRQ1 flag (bit 7 of CRB)
    if (m_cb1Line) {
        bool lvl = m_cb1Line->get();
        bool activeEdge = (m_crb & 0x01) ? (!m_cb1Prev && lvl)
                                          : (m_cb1Prev && !lvl);
        if (activeEdge) {
            m_crb |= 0x80;
            // In handshake output mode, active CB1 edge releases CB2
            if ((m_crb & 0x20) && !(m_crb & 0x10) && !(m_crb & 0x08))
                driveCB2(true);
            updateIrqB();
        }
        m_cb1Prev = lvl;
    }

    // CB2 as input: active edge sets IRQ2 flag (bit 6 of CRB)
    if (m_cb2Line && !(m_crb & 0x20)) {
        bool lvl = m_cb2Line->get();
        bool activeEdge = (m_crb & 0x08) ? (!m_cb2Prev && lvl)
                                          : (m_cb2Prev && !lvl);
        if (activeEdge) {
            m_crb |= 0x40;
            updateIrqB();
        }
        m_cb2Prev = lvl;
    }
}

// ---------------------------------------------------------------------------
// Snapshot
// ---------------------------------------------------------------------------

PIA6520::Snapshot PIA6520::getSnapshot() const {
    return {m_ora, m_ddra, m_cra, m_orb, m_ddrb, m_crb,
            m_ca1Prev, m_ca2Prev, m_cb1Prev, m_cb2Prev};
}

void PIA6520::restoreSnapshot(const Snapshot& s) {
    m_ora  = s.ora;  m_ddra = s.ddra; m_cra  = s.cra;
    m_orb  = s.orb;  m_ddrb = s.ddrb; m_crb  = s.crb;
    m_ca1Prev = s.ca1Prev; m_ca2Prev = s.ca2Prev;
    m_cb1Prev = s.cb1Prev; m_cb2Prev = s.cb2Prev;
    updateIrqA();
    updateIrqB();
}
