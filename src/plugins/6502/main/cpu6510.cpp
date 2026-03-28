#include "cpu6510.h"

MOS6510::MOS6510() {
    // Default pull-up state: DDR=0 (all inputs), DATA=0x3F (bits 0-5 high).
    // Effective port output = ~DDR & 0xFF = 0xFF → all banking lines high
    // (LORAM=1, HIRAM=1, CHAREN=1 → full C64 memory map visible at reset).
    updateSignals();
}

MOS6510::~MOS6510() {
    delete m_portBus;
}

void MOS6510::installBus(IBus* bus) {
    delete m_portBus;
    m_realBus = bus;
    if (bus) {
        m_portBus = new PortBus(this, bus);
        // Pass the proxy to the 6502 core so all CPU reads/writes go through it.
        MOS6502::setDataBus(m_portBus);
    } else {
        m_portBus = nullptr;
        MOS6502::setDataBus(nullptr);
    }
}

void MOS6510::setDataBus(IBus* bus) {
    installBus(bus);
}

void MOS6510::setCodeBus(IBus* bus) {
    // The 6510 has a single unified bus; code and data share the same proxy.
    // If the caller passes a different bus object for code, honour it only if
    // no data bus has been set yet — otherwise keep the existing proxy.
    if (!m_realBus && bus) installBus(bus);
    else if (bus == m_realBus || !bus) return;
    else installBus(bus); // caller explicitly set a different code bus
}

void MOS6510::updateSignals() {
    // Effective port level: output bits driven by DATA, input bits pulled high.
    uint8_t level = (m_data & m_ddr) | (~m_ddr & 0x3F);
    m_loram .set((level >> 0) & 1);
    m_hiram .set((level >> 1) & 1);
    m_charen.set((level >> 2) & 1);
}
