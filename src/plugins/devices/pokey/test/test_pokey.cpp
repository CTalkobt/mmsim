#include "test_harness.h"
#include "plugins/devices/pokey/main/pokey.h"
#include <vector>

class MockSignal : public ISignalLine {
public:
    bool get() const override { return m_level; }
    void set(bool level) override { m_level = level; }
    void pulse() override { m_level = !m_level; m_level = !m_level; }
    bool m_level = false;
};

TEST_CASE(pokey_registers) {
    POKEY pokey;
    uint8_t val;
    
    // Test AUDF1/AUDC1
    pokey.ioWrite(nullptr, 0xD200, 0x10);
    pokey.ioRead(nullptr, 0xD200, &val);
    ASSERT(val == 0x10);
    
    pokey.ioWrite(nullptr, 0xD201, 0xA8); // Pure tone, vol 8
    pokey.ioRead(nullptr, 0xD201, &val);
    ASSERT(val == 0xA8);
    
    // RANDOM read
    pokey.ioRead(nullptr, 0xD20B, &val);
    // Should be non-zero after reset (poly init to 1)
    ASSERT(val != 0);
}

TEST_CASE(pokey_timers) {
    POKEY pokey;
    MockSignal irq;
    pokey.setIrqLine(&irq);
    
    // Enable timer 1 IRQ
    pokey.ioWrite(nullptr, 0xD20E, 0x01); // IRQEN bit 0
    
    // Set Ch1 freq to 10
    pokey.ioWrite(nullptr, 0xD200, 10);
    
    // Start timers
    pokey.ioWrite(nullptr, 0xD209, 0x00);
    
    // Tick forward 11 cycles
    pokey.tick(11);
    
    uint8_t irqst;
    pokey.ioRead(nullptr, 0xD20E, &irqst);
    ASSERT((irqst & 0x01) == 0); // IRQST bit 0 should be 0 (active)
    ASSERT(irq.m_level == true);
    
    // Clear IRQ via SKRES (Wait, SKRES clears errors, SKCTL clears keyboard? Actually IRQST is just a status, SKRES doesn't clear it. SKRES resets serial.)
    // IRQs are cleared by writing to IRQEN bit 0 again? No, IRQEN bits 7-0 enable.
    // IRQST is read-only.
}

TEST_CASE(pokey_audio_output) {
    POKEY pokey;
    pokey.setSampleRate(44100);
    pokey.setClockHz(1789773);
    
    // Pure tone on Ch1
    pokey.ioWrite(nullptr, 0xD200, 100);
    pokey.ioWrite(nullptr, 0xD201, 0xA8); // Pure tone, vol 8
    
    // Tick enough cycles to generate some samples
    // 1789773 / 44100 ~= 40 cycles per sample
    pokey.tick(400);
    
    float buffer[100];
    int count = pokey.pullSamples(buffer, 100);
    ASSERT(count > 0);
    
    // Check that we have some non-zero samples
    bool nonZero = false;
    for (int i = 0; i < count; ++i) {
        if (buffer[i] != 0.0f) {
            nonZero = true;
            break;
        }
    }
    ASSERT(nonZero);
}
