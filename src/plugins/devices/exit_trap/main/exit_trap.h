#pragma once
#include "libdevices/main/io_handler.h"
#include <string>

class ExitTrapDevice : public IOHandler {
public:
    ExitTrapDevice(uint32_t addr = 0xD6CF);
    virtual ~ExitTrapDevice() {}

    const char* name() const override { return "45gs02 exit trap"; }
    uint32_t    baseAddr() const override { return m_baseAddr; }
    uint32_t    addrMask() const override { return 0; } // Single register

    void reset() override { m_haltRequested = false; }
    bool ioRead (IBus* bus, uint32_t addr, uint8_t* val) override { return false; }
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val)  override;

    // Tick is required by IOHandler
    void tick(uint64_t cycles) override {}

    // isHaltRequested is in IOHandler
    bool isHaltRequested() const override { return m_haltRequested; }

private:
    uint32_t m_baseAddr;
    bool     m_haltRequested = false;
};
