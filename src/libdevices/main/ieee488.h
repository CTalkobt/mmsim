#pragma once

#include "isignal_line.h"
#include "io_handler.h"
#include <vector>
#include <string>
#include <cstdint>

/**
 * IEEE-488 (GPIB) Bus interface.
 * All lines are active-low (0V = Logic 1).
 */
class IEEE488Bus {
public:
    enum Signal {
        ATN,    // Attention
        DAV,    // Data Valid
        NRFD,   // Not Ready For Data
        NDAC,   // Not Data Accepted
        EOI,    // End Or Identify
        SRQ,    // Service Request
        IFC,    // Interface Clear
        REN     // Remote Enable
    };

    virtual ~IEEE488Bus() = default;

    // Line access (physical level)
    virtual void setData(uint8_t val) = 0;
    virtual uint8_t getData() const = 0;

    virtual void setSignal(Signal sig, bool level) = 0;
    virtual bool getSignal(Signal sig) const = 0;

    // HLE Device Interface
    class Device {
    public:
        virtual ~Device() = default;
        virtual int getAddress() const = 0;
        virtual void onSignalChange(IEEE488Bus* bus, Signal sig) = 0;
        virtual void onDataWrite(IEEE488Bus* bus, uint8_t val) = 0;
    };

    virtual void registerDevice(Device* device) = 0;
};

/**
 * Concrete implementation of the IEEE-488 bus.
 */
class SimpleIEEE488Bus : public IEEE488Bus, public IOHandler {
public:
    SimpleIEEE488Bus();

    // IEEE488Bus implementation
    void setData(uint8_t val) override;
    uint8_t getData() const override;
    void setSignal(Signal sig, bool level) override;
    bool getSignal(Signal sig) const override;
    void registerDevice(Device* device) override;

    // IOHandler implementation
    const char* name() const override { return "IEEE-488"; }
    uint32_t baseAddr() const override { return 0xE810; } // Default PET location (PIA1)
    uint32_t addrMask() const override { return 0x000F; }

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override {}

private:
    uint8_t m_data = 0xFF; // All high (logic 0)
    bool m_signals[8];
    std::vector<Device*> m_devices;

    void notifyChange(Signal sig);
};
