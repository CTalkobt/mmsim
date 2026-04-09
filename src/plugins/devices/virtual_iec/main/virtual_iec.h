#pragma once

#include "libdevices/main/iport_device.h"
#include "libdevices/main/io_handler.h"
#include <string>
#include <vector>
#include <cstdint>

/**
 * Virtual IEC Bus Device (e.g. Unit 8).
 * Implements Level 2 (protocol-level) IEC serial bus emulation.
 */
class VirtualIECBus : public IPortDevice, public IOHandler {
public:
    enum State {
        IDLE,
        ATTENTION,
        ADDRESSING,
        ACKNOWLEDGE,
        READY_TO_RECEIVE,
        RECEIVING,
        READY_TO_SEND,
        SENDING,
        ERROR
    };

    VirtualIECBus(uint8_t deviceNumber = 8);
    virtual ~VirtualIECBus() {}

    // IOHandler interface
    const char* name() const override { return "VirtualIEC"; }
    uint32_t baseAddr() const override { return 0; }
    uint32_t addrMask() const override { return 0; }
    bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t) override { return false; }
    void reset() override;
    void tick(uint64_t cycles) override;

    // IPortDevice interface
    uint8_t readPort() override;
    void writePort(uint8_t val) override;
    void setDdr(uint8_t ddr) override;

    // Disk/Image management
    bool mountDisk(int unit, const std::string& path) override;
    void ejectDisk(int unit) override;

private:
    uint8_t m_deviceNumber;
    State m_state;

    // Signal state (Open-collector logic: 1 = Low/True, 0 = High/False)
    bool m_atnIn;
    bool m_clkIn;
    bool m_dataIn;
    bool m_clkOut;
    bool m_dataOut;

    // Bit/Byte transfer state
    uint8_t m_currentByte;
    int m_bitCount;
    uint64_t m_stateTimer;
    
    // Command/Data buffer
    std::vector<uint8_t> m_buffer;
    size_t m_bufferIdx;

    void handleAttention();
    void handleBitTransfer();
    void processCommand(uint8_t cmd);
};
