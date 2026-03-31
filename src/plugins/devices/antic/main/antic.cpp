#include "antic.h"
#include "libmem/main/ibus.h"
#include <cstring>

bool Antic::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    uint32_t offset = addr & 0x000F;
    switch (offset) {
        case VCOUNT:
            *val = (uint8_t)(m_vcount >> 1); // VCOUNT is 1/2 of line number
            return true;
        case NMIST:
            *val = m_nmist;
            // NMIST is not cleared on read in real hardware, it's cleared by write to NMIRES
            return true;
        default:
            *val = m_regs[offset];
            return true;
    }
}

bool Antic::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    uint32_t offset = addr & 0x000F;
    m_regs[offset] = val;

    switch (offset) {
        case DLISTL:
            m_dlistPtr = (m_dlistPtr & 0xFF00) | val;
            break;
        case DLISTH:
            m_dlistPtr = (m_dlistPtr & 0x00FF) | ((uint16_t)val << 8);
            break;
        case WSYNC:
            m_wsync = true;
            if (m_haltLine) m_haltLine->set(true);
            break;
        case NMIEN:
            updateNmi();
            break;
        case NMIST:
            m_nmist = 0; // NMIRES (Write-only alias of NMIST)
            if (m_nmiLine) m_nmiLine->set(false);
            break;
    }
    return true;
}

void Antic::reset() {
    std::memset(m_regs, 0, sizeof(m_regs));
    m_vcount = 0;
    m_hcount = 0;
    m_cycleAccum = 0;
    m_dlistPtr = 0;
    m_memoryScanAddr = 0;
    m_wsync = false;
    m_nmist = 0;
    m_lastCycles = 0;
    if (m_haltLine) m_haltLine->set(false);
    if (m_nmiLine) m_nmiLine->set(false);
}

void Antic::tick(uint64_t cycles) {
    if (cycles <= m_lastCycles) {
        m_lastCycles = cycles;
        return;
    }
    uint32_t delta = (uint32_t)(cycles - m_lastCycles);
    m_lastCycles = cycles;

    for (uint32_t i = 0; i < delta; ++i) {
        // Atari hcount is 0-113. 
        // CPU clock (1.79MHz) is exactly one ANTIC cycle.
        
        // Display List DMA
        // DL DMA happens at certain cycles if enabled.
        // Usually cycles 8, 9, 10 for instruction fetch.
        if ((m_regs[DMACTL] & DMA_DL) && m_vcount >= 8 && m_vcount < 248) {
            if (m_hcount == 8) {
                if (m_haltLine) m_haltLine->set(true);
                uint8_t op = fetchByte(m_dlistPtr++);
                
                // DLI bit handling
                if (op & 0x80) {
                    if (m_regs[NMIEN] & NMI_DLI) {
                        m_nmist |= NMI_DLI;
                        updateNmi();
                    }
                }

                // Basic DL instruction handling
                uint8_t mode = op & 0x0F;
                if (mode == 0x01) { // JMP or JSR
                    uint8_t lo = fetchByte(m_dlistPtr++);
                    uint8_t hi = fetchByte(m_dlistPtr++);
                    m_dlistPtr = lo | (hi << 8);
                } else if (op & 0x40) { // Load Memory Scan (LMS)
                    uint8_t lo = fetchByte(m_dlistPtr++);
                    uint8_t hi = fetchByte(m_dlistPtr++);
                    m_memoryScanAddr = lo | (hi << 8);
                }
                if (m_haltLine) m_haltLine->set(false);
            }
        }

        // Playfield DMA
        // Happens between cycles 10 and 104 roughly for standard width.
        // Simplified: we just increment m_memoryScanAddr if it was a mode line
        // (Full implementation would fetch scanline data here)

        m_hcount++;
        if (m_hcount >= 114) {
            m_hcount = 0;
            m_vcount++;
            if (m_vcount >= 262) {
                m_vcount = 0;
                m_dlistPtr = m_regs[DLISTL] | ((uint16_t)m_regs[DLISTH] << 8);
                // Start of Vertical Blank
                if (m_regs[NMIEN] & NMI_VBI) {
                    m_nmist |= NMI_VBI;
                    updateNmi();
                }
            }

            // End of WSYNC at start of new line
            if (m_wsync) {
                m_wsync = false;
                if (m_haltLine) m_haltLine->set(false);
            }
        }
    }
}

void Antic::renderFrame(uint32_t* buffer) {
    // Placeholder for frame rendering
    std::memset(buffer, 0, FRAME_W * FRAME_H * 4);
}

void Antic::updateNmi() {
    if (m_nmiLine) {
        bool active = (m_nmist & m_regs[NMIEN]) != 0;
        m_nmiLine->set(active);
    }
}

uint8_t Antic::fetchByte(uint16_t addr) {
    if (m_dmaBus) {
        return m_dmaBus->read8(addr);
    }
    return 0xFF;
}
