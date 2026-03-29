#include "test_harness.h"
#include "pia6520.h"

// Base addresses for PET PIA1 ($E810) and PIA2 ($E820)
static constexpr uint32_t BASE = 0xE810;

// RS offsets
static constexpr uint32_t ORA_DDRA = BASE + 0;
static constexpr uint32_t CRA_ADDR = BASE + 1;
static constexpr uint32_t ORB_DDRB = BASE + 2;
static constexpr uint32_t CRB_ADDR = BASE + 3;

class MockSignal : public ISignalLine {
public:
    bool get()   const override { return m_level; }
    void set(bool l) override   { m_level = l; }
    void pulse() override       { m_level = !m_level; m_level = !m_level; }
    bool m_level = false;
};

class MockPort : public IPortDevice {
public:
    uint8_t readPort()           override { return m_pins; }
    void    writePort(uint8_t v) override { m_written = v; }
    void    setDdr(uint8_t d)    override { m_ddr = d; }
    uint8_t m_pins   = 0xFF;
    uint8_t m_written = 0;
    uint8_t m_ddr    = 0;
};

// ---------------------------------------------------------------------------
// Test 1: DDR / OR select via CRA/CRB bit 2
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ddr_or_select) {
    PIA6520 pia("pia1", BASE);
    MockPort portA;
    pia.setPortADevice(&portA);

    // CRA bit 2 = 0 (default) → RS=00 accesses DDRA
    pia.ioWrite(nullptr, CRA_ADDR,  0x00); // CRA bit 2 = 0
    pia.ioWrite(nullptr, ORA_DDRA,  0xF0); // write DDRA = 0xF0
    ASSERT(portA.m_ddr == 0xF0);

    // CRA bit 2 = 1 → RS=00 accesses ORA
    pia.ioWrite(nullptr, CRA_ADDR,  0x04); // CRA bit 2 = 1
    pia.ioWrite(nullptr, ORA_DDRA,  0xAB); // write ORA = 0xAB
    ASSERT(portA.m_written == 0xAB);

    // Read back ORA merged with external pins (DDRA=0xF0: upper nibble output, lower nibble input)
    portA.m_pins = 0x5F;  // external pulls lower nibble to 0xF
    uint8_t val = 0;
    pia.ioRead(nullptr, ORA_DDRA, &val);
    ASSERT(val == 0xAF); // upper nibble from ORA output (0xA), lower from pins (0xF)
}

// ---------------------------------------------------------------------------
// Test 2: Port B DDR / OR select via CRB bit 2
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_portb_rw) {
    PIA6520 pia("pia1", BASE);
    MockPort portB;
    pia.setPortBDevice(&portB);

    pia.ioWrite(nullptr, CRB_ADDR,  0x00); // CRB bit 2 = 0 → DDRB
    pia.ioWrite(nullptr, ORB_DDRB,  0x0F); // DDRB = 0x0F (lower nibble output)
    ASSERT(portB.m_ddr == 0x0F);

    pia.ioWrite(nullptr, CRB_ADDR,  0x04); // CRB bit 2 = 1 → ORB
    pia.ioWrite(nullptr, ORB_DDRB,  0x37);
    ASSERT(portB.m_written == 0x37);
}

// ---------------------------------------------------------------------------
// Test 3: CA1 falling edge → IRQ1 flag set, IRQA asserted
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca1_irq) {
    PIA6520 pia("pia1", BASE);
    MockSignal irqa, ca1;
    pia.setIrqALine(&irqa);
    pia.setCA1Line(&ca1);

    // Enable CA1 interrupt (CRA bit 1 = 1) and enable ORA access (bit 2 = 1)
    pia.ioWrite(nullptr, CRA_ADDR, 0x06); // bits 2,1 = 1,1; edge = falling (bit 0 = 0)

    ca1.m_level = true;
    pia.tick(1); // prime prev

    // Falling edge
    ca1.set(false);
    pia.tick(1);

    uint8_t cra = 0;
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(cra & 0x80);        // IRQ1 flag set
    ASSERT(irqa.m_level);      // IRQA asserted

    // Reading ORA clears IRQ1 flag and de-asserts IRQA
    pia.ioWrite(nullptr, CRA_ADDR, 0x06); // re-enable OR access
    uint8_t ora = 0;
    pia.ioRead(nullptr, ORA_DDRA, &ora);
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(!(cra & 0x80));     // flag cleared
    ASSERT(!irqa.m_level);     // IRQA de-asserted
}

// ---------------------------------------------------------------------------
// Test 4: CA1 IRQ disabled — flag set but IRQA not asserted
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca1_irq_disabled) {
    PIA6520 pia("pia1", BASE);
    MockSignal irqa, ca1;
    pia.setIrqALine(&irqa);
    pia.setCA1Line(&ca1);

    // CRA bit 1 = 0: CA1 interrupt disabled
    pia.ioWrite(nullptr, CRA_ADDR, 0x04); // OR access, IRQ disabled, falling edge

    ca1.m_level = true;
    pia.tick(1);
    ca1.set(false);
    pia.tick(1);

    uint8_t cra = 0;
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(cra & 0x80);        // IRQ1 flag is still set
    ASSERT(!irqa.m_level);     // but IRQA NOT asserted (disabled)
}

// ---------------------------------------------------------------------------
// Test 5: CA2 input falling edge → IRQ2 flag set, IRQA asserted
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca2_input_irq) {
    PIA6520 pia("pia1", BASE);
    MockSignal irqa, ca2;
    pia.setIrqALine(&irqa);
    pia.setCA2Line(&ca2);

    // CRA: OR access (bit2=1), CA2 = input (bit5=0), falling edge (bit3=0)
    pia.ioWrite(nullptr, CRA_ADDR, 0x04);

    ca2.m_level = true;
    pia.tick(1);
    ca2.set(false);
    pia.tick(1);

    uint8_t cra = 0;
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(cra & 0x40);    // IRQ2 flag set
    ASSERT(irqa.m_level);  // IRQA asserted (CA2 input always enables IRQ2)

    // Reading ORA clears IRQ2 (non-independent mode: CRA bit 4 = 0)
    uint8_t ora = 0;
    pia.ioRead(nullptr, ORA_DDRA, &ora);
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(!(cra & 0x40));
    ASSERT(!irqa.m_level);
}

// ---------------------------------------------------------------------------
// Test 6: CA2 independent mode — IRQ2 not cleared by ORA read
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca2_independent_mode) {
    PIA6520 pia("pia1", BASE);
    MockSignal irqa, ca2;
    pia.setIrqALine(&irqa);
    pia.setCA2Line(&ca2);

    // CRA: OR (bit2=1), CA2 input independent (bit5=0, bit4=1), falling edge
    pia.ioWrite(nullptr, CRA_ADDR, 0x14); // bits: 5=0 4=1 3=0 2=1

    ca2.m_level = true;
    pia.tick(1);
    ca2.set(false);
    pia.tick(1);

    uint8_t cra = 0;
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(cra & 0x40);  // IRQ2 flag set

    // Reading ORA must NOT clear IRQ2 in independent mode
    uint8_t ora = 0;
    pia.ioRead(nullptr, ORA_DDRA, &ora);
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT(cra & 0x40);  // flag persists
}

// ---------------------------------------------------------------------------
// Test 7: CA2 handshake output — ORA read asserts, CA1 edge releases
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca2_handshake_output) {
    PIA6520 pia("pia1", BASE);
    MockSignal ca1, ca2;
    pia.setCA1Line(&ca1);
    pia.setCA2Line(&ca2);

    ca1.m_level = true;
    ca2.m_level = true;
    pia.tick(1);  // prime prev levels

    // CRA: OR (bit2=1), CA2 output handshake (bit5=1, bit4=0, bit3=0), falling CA1
    pia.ioWrite(nullptr, CRA_ADDR, 0x24); // 0b00100100

    // ORA read → CA2 asserted low
    uint8_t val = 0;
    pia.ioRead(nullptr, ORA_DDRA, &val);
    ASSERT(ca2.m_level == false);

    // Active CA1 falling edge → CA2 released high
    ca1.set(false);
    pia.tick(1);
    ASSERT(ca2.m_level == true);
}

// ---------------------------------------------------------------------------
// Test 8: CA2 manual output low / high
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_ca2_manual_output) {
    PIA6520 pia("pia1", BASE);
    MockSignal ca2;
    pia.setCA2Line(&ca2);

    // CA2 manual low: bits 5,4,3 = 1,1,0 → CRA = 0x30
    pia.ioWrite(nullptr, CRA_ADDR, 0x30);
    ASSERT(ca2.m_level == false);

    // CA2 manual high: bits 5,4,3 = 1,1,1 → CRA = 0x38
    pia.ioWrite(nullptr, CRA_ADDR, 0x38);
    ASSERT(ca2.m_level == true);
}

// ---------------------------------------------------------------------------
// Test 9: CB2 handshake output — ORB write asserts, CB1 edge releases
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_cb2_handshake_output) {
    PIA6520 pia("pia1", BASE);
    MockSignal cb1, cb2;
    pia.setCB1Line(&cb1);
    pia.setCB2Line(&cb2);

    cb1.m_level = true;
    cb2.m_level = true;
    pia.tick(1);

    // CRB: OR (bit2=1), CB2 output handshake (bit5=1, bit4=0, bit3=0), falling CB1
    pia.ioWrite(nullptr, CRB_ADDR, 0x24);

    // ORB write → CB2 asserted low
    pia.ioWrite(nullptr, ORB_DDRB, 0x55);
    ASSERT(cb2.m_level == false);

    // Active CB1 falling edge → CB2 released
    cb1.set(false);
    pia.tick(1);
    ASSERT(cb2.m_level == true);
}

// ---------------------------------------------------------------------------
// Test 10: Snapshot — save and restore full state
// ---------------------------------------------------------------------------

TEST_CASE(pia6520_snapshot) {
    PIA6520 pia("pia1", BASE);

    pia.ioWrite(nullptr, CRA_ADDR, 0x04);  // ORA access
    pia.ioWrite(nullptr, ORA_DDRA, 0x55);
    pia.ioWrite(nullptr, CRA_ADDR, 0x00);  // DDRA access
    pia.ioWrite(nullptr, ORA_DDRA, 0xF0);  // DDRA = 0xF0
    pia.ioWrite(nullptr, CRA_ADDR, 0x06);  // ORA + CA1 IRQ enable

    PIA6520::Snapshot snap = pia.getSnapshot();
    ASSERT(snap.ddra == 0xF0);
    ASSERT(snap.cra  == 0x06);

    // Modify state then restore
    pia.ioWrite(nullptr, CRA_ADDR, 0x00);
    pia.restoreSnapshot(snap);

    uint8_t cra = 0;
    pia.ioRead(nullptr, CRA_ADDR, &cra);
    ASSERT((cra & 0x3F) == 0x06);  // restored (flags may differ)
}
