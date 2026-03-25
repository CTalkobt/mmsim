#include "test_harness.h"
#include "via6522.h"
#include <vector>

class MockSignal : public ISignalLine {
public:
    bool get() const override { return m_level; }
    void set(bool level) override { m_level = level; m_changed = true; }
    void pulse() override { m_level = !m_level; m_level = !m_level; }
    bool m_level = false;
    bool m_changed = false;
};

TEST_CASE(via6522_timers) {
    VIA6522 via("via1", 0x9110);
    MockSignal irq;
    via.setIrqLine(&irq);

    // Test Timer 1 One-Shot
    via.ioWrite(nullptr, 0x9114, 0x05); // T1CL = 5
    via.ioWrite(nullptr, 0x9115, 0x00); // T1CH = 0 (starts timer)
    
    via.tick(3);
    uint8_t val;
    via.ioRead(nullptr, 0x9114, &val);
    ASSERT(val <= 5); // Should have ticked down

    via.tick(10); // Should underflow
    via.ioRead(nullptr, 0x911D, &val); // Read IFR
    ASSERT(val & 0x40); // T1 bit set

    // Enable T1 interrupt
    via.ioWrite(nullptr, 0x911E, 0xC0); // IER: Set bit 7 (Set) and bit 6 (T1)
    ASSERT(irq.m_level == true); // IRQ should be active now
}

TEST_CASE(via6522_ports) {
    VIA6522 via("via1", 0x9110);
    
    class MockPort : public IPortDevice {
    public:
        uint8_t readPort() override { return m_val; }
        void writePort(uint8_t val) override { m_written = val; }
        void setDdr(uint8_t ddr) override { m_ddr = ddr; }
        uint8_t m_val = 0xAA;
        uint8_t m_written = 0;
        uint8_t m_ddr = 0;
    };

    MockPort portA;
    via.setPortADevice(&portA);

    // Set DDRA to all inputs
    via.ioWrite(nullptr, 0x9113, 0x00);
    ASSERT(portA.m_ddr == 0x00);

    uint8_t val;
    via.ioRead(nullptr, 0x9111, &val); // Read ORA
    ASSERT(val == 0xAA);

    // Set DDRA to all outputs
    via.ioWrite(nullptr, 0x9113, 0xFF);
    via.ioWrite(nullptr, 0x9111, 0x55); // Write ORA
    ASSERT(portA.m_written == 0x55);
}
