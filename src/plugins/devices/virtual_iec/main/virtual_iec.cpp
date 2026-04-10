#include "virtual_iec.h"
#include "libdevices/main/io_handler.h"
#include "include/util/logging.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdio>

VirtualIECBus::VirtualIECBus(uint8_t deviceNumber)
    : m_deviceNumber(deviceNumber), m_state(IDLE), m_nextState(IDLE),
      m_atnIn(false), m_clkIn(false), m_dataIn(false),
      m_clkOut(false), m_dataOut(false),
      m_currentByte(0), m_bitCount(0), m_stateTimer(0), m_bufferIdx(0),
      m_secondaryAddress(0), m_fileIdx(0), m_eof(false)
{
}

uint8_t VirtualIECBus::readPort() {
    // Bus state (Open-collector logic: True = pulled Low = Logic 1)
    bool clkBus = m_clkIn || m_clkOut;
    bool dataBus = m_dataIn || m_dataOut;

    // C64 CIA2 Port A Mapping:
    // bit 3: ATN OUT, bit 4: CLK OUT, bit 5: DATA OUT
    // bit 6: CLK IN (Inverse of bus CLK), bit 7: DATA IN (Inverse of bus DATA)
    
    uint8_t val = 0;
    if (clkBus) val |= (1 << 6);
    if (dataBus) val |= (1 << 7);
    
    return val;
}

void VirtualIECBus::writePort(uint8_t val) {
    m_atnIn = (val >> 3) & 1;
    m_clkIn = (val >> 4) & 1;
    m_dataIn = (val >> 5) & 1;
}

void VirtualIECBus::setDdr(uint8_t ddr) {
    (void)ddr;
}

void VirtualIECBus::reset() {
    m_state = IDLE;
    m_nextState = IDLE;
    m_atnIn = false;
    m_clkIn = false;
    m_dataIn = false;
    m_clkOut = false;
    m_dataOut = false;
    m_currentByte = 0;
    m_bitCount = 0;
    m_stateTimer = 0;
    m_fileIdx = 0;
    m_eof = false;
    m_secondaryAddress = 0;
    m_filename.clear();
}

void VirtualIECBus::tick(uint64_t cycles) {
    m_stateTimer += cycles;

    for (int pass = 0; pass < 2; ++pass) {
        State before = m_state;
        switch (m_state) {
            case IDLE:
                if (m_atnIn) {
                    m_state = ATTENTION;
                    m_dataOut = false;
                    m_clkOut = false;
                    // Note: do NOT reset m_stateTimer here; cycles already spent
                    // this tick count towards the ATN response timing.
                }
                break;

            case ATTENTION:
                if (m_stateTimer > 2000) {
                    m_dataOut = true;
                    m_state = ADDRESSING;
                    m_bitCount = 0;
                    m_currentByte = 0;
                }
                break;

            case ADDRESSING:
                if (!m_atnIn) { m_state = IDLE; m_dataOut = false; break; }
                handleBitTransfer();
                if (m_bitCount == 8) {
                    processCommand(m_currentByte);
                    m_state = ACKNOWLEDGE;
                    m_dataOut = true;
                }
                break;

            case ACKNOWLEDGE:
                if (!m_atnIn) {
                    m_dataOut = false;
                    m_state = m_nextState;
                    m_bitCount = 0;
                    m_currentByte = 0;
                }
                break;

            case READY_TO_RECEIVE:
                if (m_atnIn) { m_state = ATTENTION; m_stateTimer = 0; break; }
                if (m_dataOut) {
                    if (m_clkIn) m_dataOut = false;
                } else {
                    handleBitTransfer();
                    if (m_bitCount == 8) {
                        handleByteReceived(m_currentByte);
                        m_bitCount = 0;
                        m_currentByte = 0;
                        m_dataOut = true;
                    }
                }
                break;

            case READY_TO_SEND:
                if (m_atnIn) { m_state = ATTENTION; m_stateTimer = 0; break; }
                if (!m_clkIn) {
                    m_clkOut = true;
                    m_currentByte = getNextByte();
                    m_state = SENDING;
                    m_bitCount = 0;
                }
                break;

            case SENDING:
                if (m_atnIn) { m_state = ATTENTION; m_stateTimer = 0; break; }
                if (m_clkOut) {
                    m_dataOut = (m_currentByte >> m_bitCount) & 1;
                    m_clkOut = false;
                } else {
                    if (m_clkIn) {
                        m_bitCount++;
                        if (m_bitCount == 8) {
                            m_state = READY_TO_SEND;
                            m_clkOut = false;
                        } else {
                            m_clkOut = true;
                        }
                    }
                }
                break;

            default:
                break;
        }
        if (m_state == before) break; 
    }
}

void VirtualIECBus::handleBitTransfer() {
    static bool lastClkIn = false;
    if (!lastClkIn && m_clkIn) {
        bool bit = m_dataIn;
        m_currentByte = (m_currentByte >> 1) | (bit ? 0x80 : 0x00);
        m_bitCount++;
    }
    lastClkIn = m_clkIn;
}

void VirtualIECBus::handleByteReceived(uint8_t byte) {
    if (m_secondaryAddress == 0 || m_secondaryAddress == 1) {
        m_filename += (char)byte;
    }
}

uint8_t VirtualIECBus::getNextByte() {
    if (m_fileIdx < m_fileBuffer.size()) {
        return m_fileBuffer[m_fileIdx++];
    }
    m_eof = true;
    return 0;
}

void VirtualIECBus::processCommand(uint8_t cmd) {
    uint8_t device = cmd & 0x1F;
    uint8_t action = cmd & 0xE0;

    if (device == m_deviceNumber) {
        if (action == 0x20) { // LISTEN
            m_nextState = READY_TO_RECEIVE;
            m_filename.clear();
        } else if (action == 0x40) { // TALK
            m_nextState = READY_TO_SEND;
            if (!m_filename.empty() && m_fileBuffer.empty()) {
                mountDisk(m_deviceNumber, m_filename);
            }
        } else if (action == 0x60) { // SECONDARY ADDRESS
            m_secondaryAddress = cmd & 0x0F;
        }
    } else {
        if (cmd == 0x3F) { // UNLISTEN
            m_nextState = IDLE;
        } else if (cmd == 0x5F) { // UNTALK
            m_nextState = IDLE;
        }
    }
}

bool VirtualIECBus::mountDisk(int unit, const std::string& path) {
    if (unit != m_deviceNumber) return false;
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    m_fileBuffer.clear();
    m_fileBuffer.assign((std::istreambuf_iterator<char>(file)),
                        (std::istreambuf_iterator<char>()));
    m_fileIdx = 0;
    m_eof = m_fileBuffer.empty();
    return true;
}

void VirtualIECBus::ejectDisk(int unit) {
    if (unit != m_deviceNumber) return;
    m_fileBuffer.clear();
    m_fileIdx = 0;
    m_eof = true;
}
