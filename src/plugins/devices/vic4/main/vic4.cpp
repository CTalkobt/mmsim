#include "vic4.h"
#include "libmem/main/ibus.h"
#include <cstring>

VIC4::VIC4()
    : VIC3("VIC-IV", 0xD000)
{
    m_colorRamExt.resize(32768, 0); // 32 KB internal color RAM
    reset();
}

void VIC4::reset() {
    VIC3::reset();
    std::memset(m_extRegs, 0, sizeof(m_extRegs));

    // Default VIC-IV extended registers (indexed as $D0xx - $D040)
    // $D060-$D063: SCRNPTR — screen RAM base (28-bit), default $0000
    m_extRegs[0x20] = 0x00; // SCRNPTRLSB
    m_extRegs[0x21] = 0x00; // SCRNPTRMSB
    m_extRegs[0x22] = 0x00; // SCRNPTRBNK
    m_extRegs[0x23] = 0x00; // SCRNPTRMB (bits 27-24)

    // $D064-$D065: COLPTR — colour RAM base, default $0000
    m_extRegs[0x24] = 0x00; // COLPTRLSB
    m_extRegs[0x25] = 0x00; // COLPTRMSB

    // $D068-$D06A: CHARPTR — character set base (24-bit), default $0000
    m_extRegs[0x28] = 0x00; // CHARPTRLSB
    m_extRegs[0x29] = 0x00; // CHARPTRMSB
    m_extRegs[0x2A] = 0x00; // CHARPTRBNK
}

void VIC4::tick(uint64_t cycles) {
    VIC3::tick(cycles);
}

bool VIC4::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;

    uint16_t offset = addr & 0x03FF;

    // $D048-$D07F: VIC-IV specific extended registers
    if (offset >= 0x0048 && offset <= 0x007F) {
        if (isLocked()) { *val = 0xFF; return true; }
        *val = m_extRegs[offset - 0x0040];
        return true;
    }

    // Everything else ($D000-$D047, $D080-$D3FF) handled by VIC3
    return VIC3::ioRead(bus, addr, val);
}

bool VIC4::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    if ((addr & ~addrMask()) != baseAddr()) return false;

    uint16_t offset = addr & 0x03FF;

    // $D048-$D07F: VIC-IV specific extended registers
    if (offset >= 0x0048 && offset <= 0x007F) {
        if (isLocked()) return true;
        m_extRegs[offset - 0x0040] = val;
        return true;
    }

    // Everything else handled by VIC3
    return VIC3::ioWrite(bus, addr, val);
}

uint32_t VIC4::getScreenBase() const {
    if (isLocked()) return VIC2::screenBase();
    // $D060-$D063: SCRNPTR (28-bit physical address)
    return ((uint32_t)(m_extRegs[0x23] & 0x0F) << 24) |
           ((uint32_t)m_extRegs[0x22] << 16) |
           ((uint32_t)m_extRegs[0x21] << 8) |
           (uint32_t)m_extRegs[0x20];
}

uint32_t VIC4::getCharBase() const {
    if (isLocked()) return VIC2::charBitmapBase();
    // $D068-$D06A: CHARPTR (24-bit physical address)
    return ((uint32_t)m_extRegs[0x2A] << 16) |
           ((uint32_t)m_extRegs[0x29] << 8) |
           (uint32_t)m_extRegs[0x28];
}

uint16_t VIC4::getColBase() const {
    // $D064-$D065: COLPTR (16-bit offset into 32KB colour RAM)
    return ((uint16_t)m_extRegs[0x25] << 8) | m_extRegs[0x24];
}

// ---------------------------------------------------------------------------
// Full-Colour Mode (FCM) rendering
//
// Each character cell uses 64 bytes of pixel data (8 rows × 8 pixels × 1 byte).
// Each byte is a palette index (0-255).
// Pixel value $FF selects the foreground colour from colour RAM instead.
//
// Enabled by FCLRLO ($D054 bit 1) for char numbers ≤ $FF
//      and/or FCLRHI ($D054 bit 2) for char numbers > $FF.
// CHR16 ($D054 bit 0) enables 16-bit character numbers from screen RAM.
// ---------------------------------------------------------------------------

void VIC4::renderFCM(uint32_t* buf) {
    uint32_t borderCol = getPaletteRGBA(m_regs[EXTCOL] & 0xFF);
    uint32_t bgCol     = getPaletteRGBA(m_regs[BGCOL0] & 0xFF);

    // Fill border
    for (int i = 0; i < FRAME_W * FRAME_H; ++i) buf[i] = borderCol;

    uint32_t scrBase  = getScreenBase();
    uint32_t charBase = getCharBase();
    uint16_t colBase  = getColBase();

    uint8_t ctrl54 = d054();
    bool chr16  = (ctrl54 & D054_CHR16)  != 0;
    bool fclrLo = (ctrl54 & D054_FCLRLO) != 0;
    bool fclrHi = (ctrl54 & D054_FCLRHI) != 0;

    bool h640 = (m_regs[REG_D031] & D031_H640) != 0;
    int cols = h640 ? 80 : 40;
    int charW = h640 ? 4 : 8; // pixel width per character on screen

    int bytesPerChar = chr16 ? 2 : 1;

    for (int row = 0; row < 25; ++row) {
        for (int col = 0; col < cols; ++col) {
            int cellIdx = row * cols + col;
            uint32_t scrAddr = scrBase + cellIdx * bytesPerChar;

            // Read character number
            uint16_t charNum;
            if (chr16) {
                uint8_t lo = m_dmaBus ? m_dmaBus->peek8(scrAddr) : 0;
                uint8_t hi = m_dmaBus ? m_dmaBus->peek8(scrAddr + 1) : 0;
                charNum = lo | ((uint16_t)hi << 8);
            } else {
                charNum = m_dmaBus ? m_dmaBus->peek8(scrAddr) : 0;
            }

            // Check if FCM applies to this character
            bool isFCM = (charNum <= 0xFF) ? fclrLo : fclrHi;
            if (!isFCM) continue; // Not FCM — leave as border (or VIC-III would render it)

            // Get foreground colour from colour RAM.
            // Prefer VIC2 color RAM pointer for backward compatibility
            // when colBase is 0 and the cell fits in the 1KB/2KB window.
            uint8_t fgColor = 0;
            uint16_t colAddr = colBase + cellIdx;
            if (m_colorRam && colBase == 0 && cellIdx < 2048) {
                fgColor = m_colorRam[cellIdx & 0x7FF];
            } else if (colAddr < m_colorRamExt.size()) {
                fgColor = m_colorRamExt[colAddr];
            }

            // FCM char data: 64 bytes at charBase + charNum * 64
            uint32_t dataAddr = charBase + (uint32_t)charNum * 64;

            int px = DISPLAY_X + col * charW;
            int py = DISPLAY_Y + row * 8;

            for (int r = 0; r < 8; ++r) {
                int fy = py + r;
                if (fy < DISPLAY_Y || fy >= DISPLAY_Y + DISPLAY_H) continue;

                for (int c = 0; c < 8; ++c) {
                    uint8_t pixVal = m_dmaBus ? m_dmaBus->peek8(dataAddr + r * 8 + c) : 0;

                    uint32_t color;
                    if (pixVal == 0xFF) {
                        // $FF = use foreground colour from colour RAM
                        color = getPaletteRGBA(fgColor);
                    } else if (pixVal == 0x00) {
                        color = bgCol;
                    } else {
                        color = getPaletteRGBA(pixVal);
                    }

                    if (h640) {
                        // In H640: 8 glyph pixels → 4 screen pixels (pair OR)
                        if (c & 1) continue; // skip odd pixels, handled by even
                        int fx = px + (c / 2);
                        if (fx >= DISPLAY_X && fx < DISPLAY_X + DISPLAY_W) {
                            buf[fy * FRAME_W + fx] = color;
                        }
                    } else {
                        int fx = px + c;
                        if (fx >= DISPLAY_X && fx < DISPLAY_X + DISPLAY_W) {
                            buf[fy * FRAME_W + fx] = color;
                        }
                    }
                }
            }
        }
    }

    // Sprites on top
    VIC2::renderSprites(buf);
}

void VIC4::renderFrame(uint32_t* buffer) {
    if (isLocked()) {
        VIC2::renderFrame(buffer);
        return;
    }

    // Check for FCM mode
    uint8_t ctrl54 = d054();
    if (ctrl54 & (D054_FCLRLO | D054_FCLRHI)) {
        renderFCM(buffer);
        return;
    }

    // Fall back to VIC-III rendering (80-col, bitplane, standard text)
    VIC3::renderFrame(buffer);
}
