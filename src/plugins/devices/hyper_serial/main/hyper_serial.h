#pragma once

#include "libdevices/main/io_handler.h"
#include <vector>
#include <string>

/**
 * MEGA65 Hyper Serial Logger.
 * Intercepts writes to $D6C1 and logs them to a file.
 */
class HyperSerialLogger : public IOHandler {
public:
    HyperSerialLogger();
    virtual ~HyperSerialLogger();

    const char* name() const override { return "hyper_serial"; }
    uint32_t baseAddr() const override { return m_baseAddr; }
    uint32_t addrMask() const override { return 0x000F; } // 16 bytes range

    void setBaseAddr(uint32_t addr) override { m_baseAddr = addr; }

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;

    void reset() override;
    void tick(uint64_t cycles) override {}

    bool isHaltRequested() const { return m_haltRequested; }

    void setLogFile(const std::string& path);

private:
    uint32_t m_baseAddr;
    FILE* m_logFile;
    std::string m_logPath;
    bool m_haltRequested = false;
};
