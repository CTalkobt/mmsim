#include "test_harness.h"
#include "ieee488.h"

class MockIEEEDevice : public IEEE488Bus::Device {
public:
    int getAddress() const override { return 8; }
    void onSignalChange(IEEE488Bus* bus, IEEE488Bus::Signal sig) override {
        signalChanges++;
        lastSignal = sig;
    }
    void onDataWrite(IEEE488Bus* bus, uint8_t val) override {
        dataWrites++;
        lastData = val;
    }

    int signalChanges = 0;
    int dataWrites = 0;
    IEEE488Bus::Signal lastSignal;
    uint8_t lastData = 0;
};

TEST_CASE(ieee488_bus_basic) {
    SimpleIEEE488Bus bus;
    MockIEEEDevice device;
    bus.registerDevice(&device);

    bus.setData(0x41);
    ASSERT(bus.getData() == 0x41);
    ASSERT(device.dataWrites == 1);
    ASSERT(device.lastData == 0x41);

    bus.setSignal(IEEE488Bus::ATN, false);
    ASSERT(bus.getSignal(IEEE488Bus::ATN) == false);
    ASSERT(device.signalChanges == 1);
    ASSERT(device.lastSignal == IEEE488Bus::ATN);
}

TEST_CASE(ieee488_io_access) {
    SimpleIEEE488Bus bus;
    uint8_t val = 0;

    // ioWrite to baseAddr should set data
    bus.ioWrite(nullptr, 0xE810, 0x55);
    ASSERT(bus.getData() == 0x55);

    // ioRead from baseAddr should get data
    bus.ioRead(nullptr, 0xE810, &val);
    ASSERT(val == 0x55);
}
