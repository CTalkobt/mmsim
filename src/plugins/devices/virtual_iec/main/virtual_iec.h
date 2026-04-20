#pragma once

#include "libdevices/main/iport_device.h"
#include "libdevices/main/io_handler.h"
#include <string>
#include <vector>
#include <cstdint>

class VirtualIECBus : public IPortDevice, public IOHandler {
public:
    enum State {
        IDLE,
        ATTENTION,
        ADDRESSING,
        ACKNOWLEDGE,
        LISTENING,
        TURNAROUND,
        TALK_READY,
        TALKING,
        TALK_FRAME,
        ERROR
    };

    VirtualIECBus(uint8_t deviceNumber = 8);
    virtual ~VirtualIECBus() {}

    uint8_t readPort() override;
    void writePort(uint8_t val) override;
    void setDdr(uint8_t ddr) override;

    const char* name() const override { return "VirtualIEC"; }
    uint32_t baseAddr() const override { return 0; }
    uint32_t addrMask() const override { return 0; }
    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override { return false; }
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override { return false; }
    void reset() override;
    void tick(uint64_t cycles) override;
    void getDeviceInfo(DeviceInfo& out) const override;

    bool mountDisk(int unit, const std::string& path) override;
    void ejectDisk(int unit) override;
    bool getDiskStatus(int unit, int& track, int& sector, bool& led) override;
    std::string getMountedDiskPath(int unit) override;

    void setLogger(void* logger, void (*logFn)(void*, int, const char*)) {
        m_logger = logger;
        m_logNamed = logFn;
    }

private:
    uint8_t m_deviceNumber;
    State m_state;
    State m_nextState;
    bool  m_isAddressed;
    bool  m_expectAddressingByte;
    bool  m_readyToSample;

    bool m_atnIn;
    bool m_clkIn;
    bool m_dataIn;
    bool m_clkOut;
    bool m_dataOut;
    bool m_lastClkIn;

    uint8_t m_currentByte;
    int m_bitCount;
    uint64_t m_stateTimer;
    bool m_isLastByte;
    int m_talkSubPhase;
    
    uint8_t m_secondaryAddress;
    std::string m_filename;
    std::string m_mountedPath;
    std::vector<uint8_t> m_fileBuffer;
    size_t m_fileIdx;
    bool m_isSendingDir;

    int  m_track;
    int  m_sector;
    bool m_led;
    uint8_t m_lastWriteVal = 0xFF;
    int m_bytesSent = 0;
    bool m_lastDataIn = false;

    void* m_logger = nullptr;
    void (*m_logNamed)(void*, int, const char*) = nullptr;

    void handleBitTransfer();
    void handleByteReceived(uint8_t byte);
    uint8_t getNextByte();
    void processCommand(uint8_t cmd);
    void generateDirectoryListing();
    void log(const char* fmt, ...);
    void logDebug(const char* fmt, ...);
};
