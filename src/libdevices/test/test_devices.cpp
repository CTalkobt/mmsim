#include "test_harness.h"
#include "libdevices/main/libdevices.h"
#include <vector>

class MockDevice : public IOHandler {
public:
    MockDevice(std::string name, uint32_t base, uint32_t mask) 
        : m_name(name), m_base(base), m_mask(mask) {}

    std::string name() const override { return m_name; }
    uint32_t baseAddr() const override { return m_base; }
    uint32_t addrMask() const override { return m_mask; }

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override {
        (void)bus;
        if ((addr & ~m_mask) == m_base) {
            *val = m_val;
            return true;
        }
        return false;
    }

    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override {
        (void)bus;
        if ((addr & ~m_mask) == m_base) {
            m_val = val;
            m_written = true;
            return true;
        }
        return false;
    }

    void reset() override { m_val = 0; m_resetCalled = true; }
    void tick(uint64_t cycles) override { m_ticks += cycles; }

    uint8_t m_val = 0;
    bool m_written = false;
    bool m_resetCalled = false;
    uint64_t m_ticks = 0;
    std::string m_name;
    uint32_t m_base, m_mask;
};

TEST_CASE(io_registry_dispatch) {
    IORegistry reg;
    MockDevice dev1("Dev1", 0xD000, 0x0FFF); // Handles D000-DFFF
    MockDevice dev2("Dev2", 0xE000, 0x0FFF); // Handles E000-EFFF

    reg.registerHandler(&dev1);
    reg.registerHandler(&dev2);

    uint8_t val = 0;
    // Test write to Dev1
    ASSERT(reg.dispatchWrite(nullptr, 0xD020, 0x42));
    ASSERT(dev1.m_written);
    ASSERT(dev1.m_val == 0x42);

    // Test read from Dev1
    ASSERT(reg.dispatchRead(nullptr, 0xD020, &val));
    ASSERT(val == 0x42);

    // Test write to Dev2
    ASSERT(reg.dispatchWrite(nullptr, 0xE005, 0x11));
    ASSERT(dev2.m_written);
    ASSERT(dev2.m_val == 0x11);

    // Test miss
    ASSERT(!reg.dispatchWrite(nullptr, 0xC000, 0x00));
}

TEST_CASE(io_registry_lifecycle) {
    IORegistry reg;
    MockDevice dev1("Dev1", 0xD000, 0x0FFF);
    reg.registerHandler(&dev1);

    reg.tickAll(100);
    ASSERT(dev1.m_ticks == 100);

    reg.resetAll();
    ASSERT(dev1.m_resetCalled);
    ASSERT(dev1.m_val == 0);
}
