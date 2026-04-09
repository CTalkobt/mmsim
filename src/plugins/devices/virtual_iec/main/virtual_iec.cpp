#include "virtual_iec.h"
#include "libdevices/main/io_handler.h"
#include "include/util/logging.h"

VirtualIECBus::VirtualIECBus(uint8_t deviceNumber)
    : m_deviceNumber(deviceNumber), m_state(IDLE),
      m_atnIn(false), m_clkIn(false), m_dataIn(false),
      m_clkOut(false), m_dataOut(false),
      m_currentByte(0), m_bitCount(0), m_stateTimer(0), m_bufferIdx(0)
{
}

uint8_t VirtualIECBus::readPort() {
    // Bus state (Open-collector logic: True = pulled Low = Logic 1)
    bool clkBus = m_clkIn || m_clkOut;
    bool dataBus = m_dataIn || m_dataOut;

    // C64 CIA2 Port A Mapping:
    // bits 0-1: VIC-II Bank Select (read-back what was written?)
    // bit 2: User Port
    // bit 3: ATN OUT (Output only from Host)
    // bit 4: CLK OUT (Output from Host)
    // bit 5: DATA OUT (Output from Host)
    // bit 6: CLK IN (Inverse of bus CLK)
    // bit 7: DATA IN (Inverse of bus DATA)
    
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
    // We don't really care about DDR for the virtual device,
    // as it's an external peripheral.
}

void VirtualIECBus::reset() {
    m_state = IDLE;
    m_atnIn = false;
    m_clkIn = false;
    m_dataIn = false;
    m_clkOut = false;
    m_dataOut = false;
    m_currentByte = 0;
    m_bitCount = 0;
    m_stateTimer = 0;
}

void VirtualIECBus::tick(uint64_t cycles) {
    m_stateTimer += cycles;

    // Process up to two state transitions per tick so that, e.g., an
    // IDLE→ATTENTION transition can immediately satisfy the ATTENTION timer
    // with the same cycle count.
    for (int pass = 0; pass < 2; ++pass) {
        State before = m_state;
        switch (m_state) {
            case IDLE:
                if (m_atnIn) {
                    m_state = ATTENTION;
                    // Do NOT reset m_stateTimer here: the cycles already
                    // accumulated this tick count towards the ATN response delay.
                    m_dataOut = false;
                    m_clkOut = false;
                }
                break;

            case ATTENTION:
                // Device pulls DATA low after ~2000 cycles to acknowledge ATN
                if (m_stateTimer > 2000) {
                    m_dataOut = true;
                    m_state = ADDRESSING;
                    m_bitCount = 0;
                    m_currentByte = 0;
                }
                break;

            case ADDRESSING:
                if (!m_atnIn) {
                    m_state = IDLE;
                    m_dataOut = false;
                    break;
                }
                handleBitTransfer();
                if (m_bitCount == 8) {
                    processCommand(m_currentByte);
                    m_state = ACKNOWLEDGE;
                    m_dataOut = true;
                    m_bitCount = 0;
                    m_currentByte = 0;
                }
                break;

            case ACKNOWLEDGE:
                if (!m_atnIn) {
                    m_dataOut = false;
                    m_state = IDLE;
                }
                break;

            default:
                break;
        }
        if (m_state == before) break; // no transition — stop early
    }
}

void VirtualIECBus::handleBitTransfer() {
    // Standard IEC Bit transfer (Host is Talker, Device is Listener):
    // 1. Host releases CLK (High/False).
    // 2. Device pulls DATA (Low/True) to acknowledge "Ready for Bit".
    // 3. Host pulls CLK (Low/True).
    // 4. Host puts DATA bit on bus.
    // 5. Host releases CLK (High/False).
    // 6. Device samples DATA and releases DATA (Low/True -> High/False) to acknowledge "Got Bit".
    
    static bool lastClkIn = false;
    
    // Simple state-less bit sampling for now (Level-based for 15.2 Level 2)
    // In a real device, this would be more complex.
    if (!lastClkIn && m_clkIn) {
        // CLK went High (released) -> Low (pulled)
        // Wait, C64 logic: bit 4 = 1 means CLK pulled Low (True).
        // So CLK transition 0 -> 1 is CLK going True.
        
        // Sample DATA bit (inverted logic)
        bool bit = m_dataIn;
        m_currentByte = (m_currentByte >> 1) | (bit ? 0x80 : 0x00);
        m_bitCount++;
    }
    
    lastClkIn = m_clkIn;
}

void VirtualIECBus::processCommand(uint8_t cmd) {
    uint8_t device = cmd & 0x1F;
    uint8_t action = cmd & 0xE0;

    if (device == m_deviceNumber) {
        if (action == 0x20) { // LISTEN
            m_state = READY_TO_RECEIVE;
        } else if (action == 0x40) { // TALK
            m_state = READY_TO_SEND;
        } else if (action == 0x60) { // SECONDARY ADDRESS
            // Handle secondary address
        }
    } else {
        // Not for us
        if (action == 0x3F) { // UNLISTEN
            m_state = IDLE;
        } else if (action == 0x5F) { // UNTALK
            m_state = IDLE;
        }
    }
}

bool VirtualIECBus::mountDisk(int unit, const std::string& path) {
    if (unit != m_deviceNumber) return false;
    // Stub for now
    return true;
}

void VirtualIECBus::ejectDisk(int unit) {
    if (unit != m_deviceNumber) return;
    // Stub for now
}
