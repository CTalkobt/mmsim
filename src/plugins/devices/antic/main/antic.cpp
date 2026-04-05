#include "antic.h"
#include "gtia.h"
#include "libmem/main/ibus.h"
#include <iostream>
#include <cstring>
#include <algorithm>

bool Antic::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    uint8_t reg = addr & 0x0F;
    switch (reg) {
        case VCOUNT:
            *val = m_regs[VCOUNT];
            return true;
        case NMIST:
            *val = m_nmist;
            return true;
        default:
            *val = m_regs[reg];
            return true;
    }
}

bool Antic::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    uint8_t reg = addr & 0x0F;
    m_regs[reg] = val;

    switch (reg) {
        case WSYNC:
            m_wsync = true;
            setHalt(true);
            break;
        case DLISTL:
        case DLISTH:
            m_dlistPtr = m_regs[DLISTL] | ((uint16_t)m_regs[DLISTH] << 8);
            break;
        case NMIST: // NMIRES
            m_nmist &= 0x1F; // Top 3 bits cleared on write (DLI, VBI, Keyboard)
            updateNmi();
            break;
        case NMIEN:
            updateNmi();
            break;
        default:
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
    m_nmist = 0x1F;
    m_halted = false;
    if (m_haltLine) m_haltLine->set(false);
}

void Antic::setHalt(bool halt) {
    if (m_halted == halt) return;
    m_halted = halt;
    if (m_haltLine) {
        m_haltLine->set(halt);
    }
}

void Antic::tick(uint64_t cycles) {
    uint64_t delta = cycles;
    m_lastCycles = cycles;

    for (uint32_t i = 0; i < delta; ++i) {
        uint16_t prevVcount = m_vcount;

        // Display List DMA
        if ((m_regs[DMACTL] & DMA_DL) && m_vcount >= 8 && m_vcount < 248) {
            if (m_hcount == 8) {
                setHalt(true);
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
                setHalt(m_wsync); 
            }
        }

        m_hcount++;
        if (m_hcount >= 114) {
            m_hcount = 0;
            m_vcount++;
            
            if (m_vcount == 248 && prevVcount != 248) { // Start of VBLANK
                if (!(m_nmist & NMI_VBI)) {
                    m_nmist |= NMI_VBI;
                    updateNmi();
                    if (m_regs[NMIEN] & NMI_VBI) {
                        if (m_nmiLine) m_nmiLine->pulse();
                    }
                }
            }

            if (m_vcount >= 262) {
                m_vcount = 0;
                // NMIST bits stay set until cleared by software (NMIRES)
                m_dlistPtr = m_regs[DLISTL] | ((uint16_t)m_regs[DLISTH] << 8);
            }

            m_regs[VCOUNT] = (uint8_t)(m_vcount >> 1);

            // End of WSYNC at start of new line
            if (m_wsync) {
                m_wsync = false;
                setHalt(false);
            }
        }
    }
}

void Antic::renderFrame(uint32_t* buffer) {
    uint32_t bgColor = 0xFF000000;
    uint32_t pfColors[4] = {0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFF000000}; 
    if (m_gtia) {
        bgColor = GTIA::getRGBA(m_gtia->getReg(GTIA::COLBK));
        pfColors[0] = GTIA::getRGBA(m_gtia->getReg(GTIA::COLPF0));
        pfColors[1] = GTIA::getRGBA(m_gtia->getReg(GTIA::COLPF1));
        pfColors[2] = GTIA::getRGBA(m_gtia->getReg(GTIA::COLPF2));
        pfColors[3] = GTIA::getRGBA(m_gtia->getReg(GTIA::COLPF3));
    }

    // Default: clear to background
    for (int i = 0; i < FRAME_W * FRAME_H; ++i) buffer[i] = bgColor;

    // Bit 5 ($20) is Display List DMA. Bit 7 is unused on real hardware.
    if (!(m_regs[DMACTL] & DMA_DL)) {
        return; // Background is already drawn
    }
    if (!m_dmaBus) {
        return;
    }

    uint16_t dlPtr = m_regs[DLISTL] | (m_regs[DLISTH] << 8);
    uint16_t scanAddr = 0;
    
    int y = 0;
    int startY = 16; 

    while (y < 240) {
        uint8_t op = m_dmaBus->read8(dlPtr++);
        uint8_t mode = op & 0x0F;
        
        if (mode == 0x00) { // Blank lines
            y += ((op >> 4) & 0x07) + 1;
            continue;
        }
        
        if (mode == 0x01) { // Jump
            uint16_t target = m_dmaBus->read8(dlPtr) | (m_dmaBus->read8(dlPtr + 1) << 8);
            dlPtr = target;
            if (op & 0x40) break; // JVB
            continue;
        }

        if (op & 0x40) { // LMS
            uint16_t lo = m_dmaBus->read8(dlPtr++);
            uint16_t hi = m_dmaBus->read8(dlPtr++);
            scanAddr = lo | (hi << 8);
        }

        int height = 1;
        int widthChars = 40;
        bool isText = true;
        
        switch (mode) {
            case 0x02: height = 8;  widthChars = 40; isText = true;  break;
            case 0x03: height = 10; widthChars = 40; isText = true;  break;
            case 0x04: height = 8;  widthChars = 40; isText = true;  break;
            case 0x05: height = 16; widthChars = 40; isText = true;  break;
            case 0x06: height = 8;  widthChars = 20; isText = true;  break;
            case 0x07: height = 16; widthChars = 20; isText = true;  break;
            case 0x08: height = 8;  widthChars = 10; isText = false; break;
            case 0x09: height = 4;  widthChars = 10; isText = false; break;
            case 0x0A: height = 4;  widthChars = 20; isText = false; break;
            case 0x0B: height = 2;  widthChars = 20; isText = false; break;
            case 0x0C: height = 2;  widthChars = 20; isText = false; break;
            case 0x0D: height = 2;  widthChars = 40; isText = false; break;
            case 0x0E: height = 1;  widthChars = 40; isText = false; break;
            case 0x0F: height = 1;  widthChars = 40; isText = false; break;
            default:   height = 1;  widthChars = 0;  break;
        }

        for (int row = 0; row < height && y < 240; ++row, ++y) {
            int outY = y + startY;
            if (outY < 0 || outY >= FRAME_H) continue;

            if (isText) {
                for (int x = 0; x < widthChars; ++x) {
                    uint8_t charCode = m_dmaBus->read8(scanAddr + x);
                    uint16_t fontAddr = (m_regs[CHBASE] << 8) | (charCode << 3) | (row & 0x07);
                    uint8_t glyphLine = m_dmaBus->read8(fontAddr);

                    int pixelScale = (widthChars == 20) ? 16 : 8;
                    int xOffset = (FRAME_W - (widthChars * pixelScale)) / 2;

                    for (int bit = 0; bit < 8; ++bit) {
                        if (glyphLine & (0x80 >> bit)) {
                            uint32_t fg = pfColors[1];
                            int screenX = xOffset + x * pixelScale + (bit * (pixelScale / 8));
                            for (int s = 0; s < (pixelScale / 8) && screenX + s < FRAME_W; ++s) {
                                buffer[outY * FRAME_W + screenX + s] = fg;
                            }
                        }
                    }
                }
            } else {
                int bytesPerLine = (mode >= 0x0D) ? 40 : 20; 
                if (mode == 0x08 || mode == 0x09) bytesPerLine = 10;
                int pixelScale = FRAME_W / (bytesPerLine * 8); 
                for (int b = 0; b < bytesPerLine; ++b) {
                    uint8_t data = m_dmaBus->read8(scanAddr + b);
                    for (int bit = 0; bit < 8; ++bit) {
                        if (data & (0x80 >> bit)) {
                            uint32_t color = pfColors[1];
                            int screenX = 32 + (b * 8 + bit) * pixelScale;
                            for (int s = 0; s < pixelScale && screenX + s < FRAME_W; ++s) {
                                buffer[outY * FRAME_W + screenX + s] = color;
                            }
                        }
                    }
                }
            }
        }
        if (isText) scanAddr += widthChars;
        else {
            if (mode >= 0x0D) scanAddr += 40;
            else if (mode >= 0x0A) scanAddr += 20;
            else scanAddr += 10;
        }
    }
}

void Antic::updateNmi() {
    if (m_nmiLine) {
        bool active = (m_nmist & m_regs[NMIEN]) != 0;
        m_nmiLine->set(active);
    }
}

uint8_t Antic::fetchByte(uint16_t addr) {
    if (!m_dmaBus) return 0;
    return m_dmaBus->read8(addr);
}
