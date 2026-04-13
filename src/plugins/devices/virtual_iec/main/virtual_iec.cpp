#include "virtual_iec.h"
#include "include/util/logging.h"
#include "plugins/cbm-loader/main/d64_parser.h"
#include "plugins/cbm-loader/main/t64_parser.h"
#include "plugins/cbm-loader/main/g64_parser.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdio>
#include <algorithm>
#include <cstdarg>

VirtualIECBus::VirtualIECBus(uint8_t deviceNumber)
    : m_deviceNumber(deviceNumber), m_state(IDLE), m_nextState(IDLE),
      m_isAddressed(false), m_expectAddressingByte(false), m_readyToSample(false),
      m_atnIn(false), m_clkIn(false), m_dataIn(false),
      m_clkOut(false), m_dataOut(false), m_lastClkIn(false),
      m_currentByte(0), m_bitCount(0), m_stateTimer(0),
      m_secondaryAddress(0), m_fileIdx(0), m_isSendingDir(false),
      m_track(0), m_sector(0), m_led(false)
{
}

uint8_t VirtualIECBus::readPort() {
    bool clkBus = m_clkIn || m_clkOut;
    bool dataBus = m_dataIn || m_dataOut;
    uint8_t val = 0;
    if (!clkBus) val |= (1 << 6);
    if (!dataBus) val |= (1 << 7);
    if (!m_atnIn) val |= (1 << 3);
    return val;
}

void VirtualIECBus::writePort(uint8_t val) {
    m_atnIn = (val >> 3) & 1;
    m_clkIn = (val >> 4) & 1;
    m_dataIn = (val >> 5) & 1;
}

void VirtualIECBus::setDdr(uint8_t ddr) { (void)ddr; }

void VirtualIECBus::reset() {
    m_state = IDLE;
    m_nextState = IDLE;
    m_isAddressed = false;
    m_clkOut = false; m_dataOut = false;
    m_lastClkIn = false; m_currentByte = 0; m_bitCount = 0;
    m_stateTimer = 0; m_fileIdx = 0; m_isSendingDir = false;
    m_filename.clear(); m_track = 0; m_sector = 0; m_led = false;
}

void VirtualIECBus::log(const char* fmt, ...) {
    char buf[256]; va_list args; va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
    if (m_logger && m_logNamed) {
        m_logNamed(m_logger, SIM_LOG_INFO, buf);
    } else {
        fprintf(stderr, "[VirtualIEC] %s\n", buf);
        fflush(stderr);
    }
}

void VirtualIECBus::tick(uint64_t cycles) {
    m_stateTimer += cycles;
    switch (m_state) {
        case IDLE:
            if (m_atnIn) { m_state = ATTENTION; m_dataOut = true; m_stateTimer = 0; }
            break;
        case ATTENTION:
            // Hold DATA low as ATN acknowledge. Transition to ADDRESSING
            // when host starts clocking bits (CLK pulled).
            if (m_stateTimer > 1000 && m_clkIn) {
                m_state = ADDRESSING; m_bitCount = 0; m_currentByte = 0;
                m_stateTimer = 0; m_lastClkIn = m_clkIn; m_readyToSample = true;
            }
            break;
        case ADDRESSING:
            if (!m_atnIn) { m_dataOut = false; m_state = m_nextState; m_stateTimer = 0; break; }
            if (m_clkIn) m_readyToSample = true;
            if (m_readyToSample) handleBitTransfer();
            if (m_bitCount == 8) {
                processCommand(m_currentByte);
                m_state = ACKNOWLEDGE; m_dataOut = true; m_stateTimer = 0;
            }
            break;
        case ACKNOWLEDGE:
            if (m_atnIn) {
                if (m_clkIn) { m_dataOut = false; m_stateTimer = 0; }
                else if (!m_dataOut && m_stateTimer >= 500) {
                    m_state = ADDRESSING; m_bitCount = 0; m_currentByte = 0;
                    m_lastClkIn = m_clkIn; m_readyToSample = false;
                }
            } else if (m_stateTimer > 500) {
                m_dataOut = false; m_state = m_nextState;
                if (m_state == LISTENING) m_clkOut = true;
                m_stateTimer = 0;
            }
            break;
        case LISTENING:
            if (m_atnIn) { m_state = ATTENTION; m_dataOut = true; m_stateTimer = 0; break; }
            if (m_clkOut) { if (m_stateTimer > 1000) { m_clkOut = false; m_stateTimer = 0; m_readyToSample = false; } }
            else {
                if (m_clkIn) m_readyToSample = true;
                if (m_readyToSample) handleBitTransfer();
                if (m_bitCount == 8) {
                    handleByteReceived(m_currentByte);
                    m_state = ACKNOWLEDGE; m_dataOut = true; m_clkOut = true; m_stateTimer = 0;
                }
            }
            break;
        case TALKING_WAIT:
            if (m_atnIn) { m_state = ATTENTION; m_dataOut = true; m_stateTimer = 0; break; }
            m_dataOut = true;
            if (m_clkIn) m_readyToSample = true;
            if (m_readyToSample && !m_clkIn) {
                m_state = TALKING; m_bitCount = 0; m_currentByte = getNextByte();
                m_clkOut = true; m_dataOut = ((m_currentByte >> 0) & 1); m_stateTimer = 0;
            }
            break;
        case TALKING:
            if (m_atnIn) { m_state = ATTENTION; m_dataOut = true; m_stateTimer = 0; break; }
            if (m_clkOut) {
                if (m_stateTimer > 500) {
                    m_clkOut = false; m_stateTimer = 0;
                }
            } else {
                if (m_stateTimer > 500) {
                    m_bitCount++;
                    if (m_bitCount == 8) {
                        m_state = TALKING_WAIT; m_readyToSample = m_clkIn;
                    } else {
                        m_clkOut = true; m_dataOut = ((m_currentByte >> m_bitCount) & 1);
                        m_stateTimer = 0;
                    }
                }
            }
            break;
        default: break;
    }
    m_led = (m_state != IDLE);
}

void VirtualIECBus::handleBitTransfer() {
    if (m_lastClkIn && !m_clkIn) {
        m_currentByte = (m_currentByte >> 1) | (m_dataIn ? 0x80 : 0x00);
        m_bitCount++;
    }
    m_lastClkIn = m_clkIn;
}

void VirtualIECBus::handleByteReceived(uint8_t byte) {
    if (m_secondaryAddress == 0 || m_secondaryAddress == 1) m_filename += (char)byte;
}

uint8_t VirtualIECBus::getNextByte() {
    return (m_fileIdx < m_fileBuffer.size()) ? m_fileBuffer[m_fileIdx++] : 0;
}

void VirtualIECBus::processCommand(uint8_t cmd) {
    uint8_t device = cmd & 0x1F, action = cmd & 0xE0;

    if (action == 0x20) { // LISTEN
        if (device == m_deviceNumber) {
            m_isAddressed = true;
            m_nextState = LISTENING;
            m_filename.clear();
        } else {
            m_isAddressed = false;
        }
    } else if (action == 0x40) { // TALK
        if (device == m_deviceNumber) {
            m_isAddressed = true;
            m_nextState = TALKING_WAIT;
            if (m_filename == "$" || m_filename == "*") generateDirectoryListing();
            else if (!m_filename.empty()) {
                std::string ext = m_mountedPath.substr(m_mountedPath.find_last_of('.') + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                ICbmDiskImage* p = (ext == "d64") ? (ICbmDiskImage*)new D64Parser() : (ext == "t64") ? (ICbmDiskImage*)new T64Parser() : (ext == "g64") ? (ICbmDiskImage*)new G64Parser() : nullptr;
                if (p && p->open(m_mountedPath)) p->readFile(m_filename, m_fileBuffer);
                m_fileIdx = 0; delete p;
            }
        } else {
            m_isAddressed = false;
        }
    } else if (action == 0x60) { // SECONDARY ADDRESS
        if (m_isAddressed) {
            m_secondaryAddress = cmd & 0x0F;
        }
    } else if (cmd == 0x3F) { // UNLISTEN
        if (m_nextState == LISTENING) m_nextState = IDLE;
        m_isAddressed = false;
    } else if (cmd == 0x5F) { // UNTALK
        if (m_nextState == TALKING_WAIT || m_nextState == TALKING) m_nextState = IDLE;
        m_isAddressed = false;
    }
}

bool VirtualIECBus::mountDisk(int unit, const std::string& path) {
    if (unit != m_deviceNumber) return false;
    m_mountedPath = path; return true;
}

void VirtualIECBus::ejectDisk(int unit) { if (unit == m_deviceNumber) { m_mountedPath.clear(); m_fileBuffer.clear(); } }
bool VirtualIECBus::getDiskStatus(int unit, int& t, int& s, bool& led) {
    if (unit != m_deviceNumber) return false;
    t = m_mountedPath.empty() ? 0 : 18;
    s = 0;
    led = m_led;
    return true;
}
std::string VirtualIECBus::getMountedDiskPath(int unit) { return (unit == m_deviceNumber) ? m_mountedPath : ""; }

void VirtualIECBus::generateDirectoryListing() {
    m_fileBuffer.clear(); m_fileIdx = 0; m_isSendingDir = true;
    m_fileBuffer.push_back(0x01); m_fileBuffer.push_back(0x08);
    m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00);
    std::string h = "\x12\"MMEMU DIRECTORY  \" 00 2A"; for (char c : h) m_fileBuffer.push_back((uint8_t)c);
    m_fileBuffer.push_back(0x00);
    std::vector<CbmDirEntry> entries;
    if (!m_mountedPath.empty()) {
        std::string ext = m_mountedPath.substr(m_mountedPath.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        ICbmDiskImage* p = (ext == "d64") ? (ICbmDiskImage*)new D64Parser() : (ext == "t64") ? (ICbmDiskImage*)new T64Parser() : (ext == "g64") ? (ICbmDiskImage*)new G64Parser() : nullptr;
        if (p && p->open(m_mountedPath)) entries = p->getDirectory();
        delete p;
    }
    for (const auto& e : entries) {
        m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00);
        m_fileBuffer.push_back(e.sizeBlocks & 0xFF); m_fileBuffer.push_back(e.sizeBlocks >> 8);
        m_fileBuffer.push_back(' '); m_fileBuffer.push_back('"');
        std::string n = e.filename; if (n.length() > 16) n = n.substr(0, 16);
        for (char c : n) m_fileBuffer.push_back((uint8_t)c);
        m_fileBuffer.push_back('"'); for (size_t i = n.length(); i < 16; ++i) m_fileBuffer.push_back(' ');
        m_fileBuffer.push_back(' '); m_fileBuffer.push_back('P'); m_fileBuffer.push_back('R'); m_fileBuffer.push_back('G');
        m_fileBuffer.push_back(0x00);
    }
    m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00);
    std::string f = "BLOCKS FREE.             "; for (char c : f) m_fileBuffer.push_back((uint8_t)c);
    m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00); m_fileBuffer.push_back(0x00);
    for (size_t p = 2; p + 2 < m_fileBuffer.size(); ) {
        size_t next = p; while (next < m_fileBuffer.size() && m_fileBuffer[next] != 0) next++;
        next++; if (next + 2 >= m_fileBuffer.size()) break;
        uint16_t link = 0x0801 + (uint16_t)next - 2;
        m_fileBuffer[p] = link & 0xFF; m_fileBuffer[p+1] = link >> 8; p = next;
    }
}

void VirtualIECBus::getDeviceInfo(DeviceInfo& out) const {
    out.name = "VirtualIEC";
    out.baseAddr = 0;
    out.addrMask = 0;

    const char* stateStr = "UNKNOWN";
    switch (m_state) {
        case IDLE: stateStr = "IDLE"; break;
        case ATTENTION: stateStr = "ATTENTION"; break;
        case ADDRESSING: stateStr = "ADDRESSING"; break;
        case ACKNOWLEDGE: stateStr = "ACKNOWLEDGE"; break;
        case LISTENING: stateStr = "LISTENING"; break;
        case TALKING_WAIT: stateStr = "TALKING_WAIT"; break;
        case TALKING: stateStr = "TALKING"; break;
        case ERROR: stateStr = "ERROR"; break;
    }
    out.state.push_back({"State", stateStr});
    out.state.push_back({"Device Number", std::to_string(m_deviceNumber)});
    out.state.push_back({"Is Addressed", m_isAddressed ? "yes" : "no"});
    
    out.state.push_back({"ATN In", m_atnIn ? "LO (asserted)" : "HI (released)"});
    out.state.push_back({"CLK In", m_clkIn ? "LO" : "HI"});
    out.state.push_back({"DATA In", m_dataIn ? "LO" : "HI"});
    out.state.push_back({"CLK Out", m_clkOut ? "LO" : "HI"});
    out.state.push_back({"DATA Out", m_dataOut ? "LO" : "HI"});

    if (!m_mountedPath.empty()) {
        out.state.push_back({"Mounted Disk", m_mountedPath});
        out.state.push_back({"Track", std::to_string(m_track)});
        out.state.push_back({"Sector", std::to_string(m_sector)});
        out.state.push_back({"Activity LED", m_led ? "ON" : "OFF"});
    } else {
        out.state.push_back({"Mounted Disk", "none"});
    }
}
