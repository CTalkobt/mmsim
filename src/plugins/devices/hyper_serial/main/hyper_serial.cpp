#include "hyper_serial.h"
#include <cstdio>
#include <cstdlib>

HyperSerialLogger::HyperSerialLogger() : m_baseAddr(0xD6C0), m_logFile(nullptr) {
    fprintf(stderr, "HyperSerialLogger created at %p\n", (void*)this);
    setLogFile("hyper_serial.log");
}

HyperSerialLogger::~HyperSerialLogger() {
    if (m_logFile) {
        fclose(m_logFile);
    }
}

void HyperSerialLogger::setLogFile(const std::string& path) {
    if (m_logFile) fclose(m_logFile);
    m_logPath = path;
    m_logFile = fopen(path.c_str(), "w");
}

void HyperSerialLogger::reset() {}

bool HyperSerialLogger::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    uint32_t offset = addr - m_baseAddr;
    
    if (offset == 0) { // $D6C0 status
        *val = 0x03; // Tx and Rx ready
        return true;
    }
    return false;
}

bool HyperSerialLogger::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    uint32_t offset = addr - m_baseAddr;
    
    if (offset == 1) { // $D6C1 data
        if (m_logFile) {
            fputc(val, m_logFile);
            fflush(m_logFile);
        }
        return true;
    }
    
    if (offset == 0x0F) { // $D6CF exit trigger
        fprintf(stderr, "HyperSerial: Exit trigger write at $%04X val=$%02X\n", addr, val);
        if (val == 0x42) {
            fprintf(stderr, "HyperSerial: Halt requested via $D6CF\n");
            m_haltRequested = true;
        }
        return true;
    }

    return false;
}
