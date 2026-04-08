#include "test_harness.h"
#include "cia6526.h"

class MockSignalLine : public ISignalLine {
public:
    void set(bool level) override {
        m_level = level;
    }
    bool get() const override {
        return m_level;
    }
    void pulse() override {
        m_level = !m_level;
        m_level = !m_level;
    }
private:
    bool m_level = false;
};

TEST_CASE(cia6526_basic_io) {
    CIA6526 cia("CIA1", 0xDC00);
    uint8_t val = 0;

    // Test DDRA and PRA
    cia.ioWrite(nullptr, 0xDC02, 0xFF); // DDRA = all output
    cia.ioWrite(nullptr, 0xDC00, 0x42); // PRA = 0x42
    cia.ioRead(nullptr, 0xDC00, &val);
    ASSERT(val == 0x42);

    // Test Timer A latching
    cia.ioWrite(nullptr, 0xDC04, 0x34); // TALO
    cia.ioWrite(nullptr, 0xDC05, 0x12); // TAHI -> should load into counter if stopped
    
    cia.ioRead(nullptr, 0xDC04, &val);
    ASSERT(val == 0x34);
    cia.ioRead(nullptr, 0xDC05, &val);
    ASSERT(val == 0x12);
}

TEST_CASE(cia6526_timer_a) {
    CIA6526 cia("CIA1", 0xDC00);
    MockSignalLine irq;
    cia.setIrqLine(&irq);

    // Set Timer A to 10 cycles, one-shot
    cia.ioWrite(nullptr, 0xDC04, 10);   // TALO
    cia.ioWrite(nullptr, 0xDC05, 0);    // TAHI
    cia.ioWrite(nullptr, 0xDC0D, 0x81); // ICR: enable Timer A interrupt (bit 7=1 to set)
    
    // Start Timer A (CRA bit 0=1, bit 3=1 for one-shot)
    cia.ioWrite(nullptr, 0xDC0E, 0x09); 

    ASSERT(irq.get() == false);

    // Tick 5 cycles
    cia.tick(5);
    uint8_t lo = 0;
    cia.ioRead(nullptr, 0xDC04, &lo);
    ASSERT(lo == 5);
    ASSERT(irq.get() == false);

    // Tick 5 more cycles -> underflow
    cia.tick(5);
    ASSERT(irq.get() == true);

    // Read ICR to clear interrupt
    uint8_t icr = 0;
    cia.ioRead(nullptr, 0xDC0D, &icr);
    ASSERT(icr & 0x81); // Should have TA and INT bits set
    ASSERT(irq.get() == false); // IRQ should be de-asserted
    
    // Verify one-shot stopped the timer
    uint8_t cra = 0;
    cia.ioRead(nullptr, 0xDC0E, &cra);
    ASSERT((cra & 0x01) == 0);
}

TEST_CASE(cia6526_icr_masking) {
    CIA6526 cia("CIA1", 0xDC00);
    MockSignalLine irq;
    cia.setIrqLine(&irq);

    // Set Timer A to 2 cycles
    cia.ioWrite(nullptr, 0xDC04, 2);
    cia.ioWrite(nullptr, 0xDC05, 0);
    
    // Do NOT enable Timer A interrupt yet
    cia.ioWrite(nullptr, 0xDC0E, 0x01); // Start continuous
    
    cia.tick(2);
    ASSERT(irq.get() == false); // Masked
    
    uint8_t icr = 0;
    cia.ioRead(nullptr, 0xDC0D, &icr);
    ASSERT(icr == 0x01); // TA bit set, but NOT bit 7 (INT) because it was masked
    
    // Now enable it and trigger again
    cia.ioWrite(nullptr, 0xDC0D, 0x81); // Enable TA
    cia.tick(2);
    ASSERT(irq.get() == true);
    
    cia.ioRead(nullptr, 0xDC0D, &icr);
    ASSERT(icr == 0x81);
}
