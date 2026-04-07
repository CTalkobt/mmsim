#include "vic2.h"
#include "libmem/main/ibus.h"
#include "mmemu_plugin_api.h"
#include <cstring>
#include <algorithm>
#include <cstdio>

VIC2::VIC2(const std::string& name, uint32_t baseAddr)
    : m_name(name), m_baseAddr(baseAddr)
{
    reset();
}

void VIC2::reset() {
    std::memset(m_regs, 0, sizeof(m_regs));
    // Power-on state: display enabled, 25 rows, 40 cols, standard text
    m_regs[SCR1] = SCR1_DEN | SCR1_RSEL | 0x03; // scroll Y = 3
    m_regs[SCR2] = SCR2_CSEL | 0x08;             // scroll X = 0, CSEL=1
    m_regs[EXTCOL] = 0x0E; // light blue border (common C64 default)
    m_regs[BGCOL0] = 0x06; // blue background
    m_regs[VMEM]   = 0x14; // screen at $0400, char at $1000

    m_cycleAccum = 0;
    m_rasterLine = 0;
    m_sprSprColl = 0;
    m_sprBgColl  = 0;

    if (m_irqLine) m_irqLine->set(false);
}

// ---------------------------------------------------------------------------
// Standard C64 palette (RGBA8888)
// ---------------------------------------------------------------------------

static const uint32_t s_palette[16] = {
    0xFF000000, // 0  Black
    0xFFFFFFFF, // 1  White
    0xFF68372B, // 2  Red
    0xFF70A4B2, // 3  Cyan
    0xFF6F3D86, // 4  Purple
    0xFF588D43, // 5  Green
    0xFF352879, // 6  Blue
    0xFFB8C76F, // 7  Yellow
    0xFF6F4F25, // 8  Orange
    0xFF433900, // 9  Brown
    0xFF9A6759, // 10 Light Red
    0xFF444444, // 11 Dark Gray
    0xFF6C6C6C, // 12 Gray
    0xFF9AD284, // 13 Light Green
    0xFF6C5EB5, // 14 Light Blue
    0xFF959595, // 15 Light Gray
};

uint32_t VIC2::palette(uint8_t index) const {
    return s_palette[index & 0x0F];
}

void VIC2::log(int level, const char* msg) const {
    if (m_logger && m_logNamed) {
        m_logNamed(m_logger, level, msg);
    }
}

// ---------------------------------------------------------------------------
// DMA access helpers
// ---------------------------------------------------------------------------

uint8_t VIC2::dmaPeek(uint32_t offset) const {
    if (!m_dmaBus) return 0xFF;
    offset &= 0x3FFF; // 14-bit internal VIC-II address

    // Character ROM shadow: bank offsets $1000-$1FFF.
    // On a real C64, this shadow is ONLY visible in banks 0 and 2
    // (where the VIC-II's VA13 pin is effectively 0).
    bool isBank0or2 = (m_bankBase == 0x0000 || m_bankBase == 0x8000);
    if (m_charRom && isBank0or2 && (offset >= 0x1000 && offset <= 0x1FFF)) {
        return m_charRom[offset & 0x0FFF];
    }

    uint32_t addr = (m_bankBase + offset) & 0xFFFF;
    return m_dmaBus->peek8(addr);
}

uint8_t VIC2::charRomByte(uint32_t offset) const {
    if (!m_charRom || m_charRomSize == 0) return 0xFF;
    return m_charRom[offset & 0x0FFF];
}

// ---------------------------------------------------------------------------
// Memory pointer helpers
// ---------------------------------------------------------------------------

uint32_t VIC2::screenBase() const {
    // VMEM bits 7-4 select screen RAM in 1 KB steps within the 16 KB VIC bank.
    return ((m_regs[VMEM] >> 4) & 0x0F) * 0x0400;
}

uint32_t VIC2::charBitmapBase() const {
    // VMEM bits 3-1 select char/bitmap base in 2 KB steps within the 16 KB VIC bank.
    return ((m_regs[VMEM] >> 1) & 0x07) * 0x0800;
}

// ---------------------------------------------------------------------------
// IOHandler: ioRead
// ---------------------------------------------------------------------------

bool VIC2::ioRead(IBus* /*bus*/, uint32_t addr, uint8_t* val) {
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    uint8_t reg = addr & 0x3F;

    if (reg > 0x2E) {
        // $D02F–$D03F: unused, read as $FF
        *val = 0xFF;
        return true;
    }

    switch (reg) {
        case RASTER:
            *val = (uint8_t)(m_rasterLine & 0xFF);
            break;
        case SCR1:
            *val = (m_regs[SCR1] & ~SCR1_RASTER8) |
                   ((m_rasterLine >> 8) ? SCR1_RASTER8 : 0);
            break;
        case IRQ:
            // Bit 7 is set if any enabled IRQ source is active.
            *val = m_regs[IRQ] | ((m_regs[IRQ] & m_regs[IRQEN] & 0x0F) ? IRQ_ANY : 0);
            break;
        case SSCOL:
            *val = m_sprSprColl;
            m_sprSprColl = 0; // cleared on read
            break;
        case SBCOL:
            *val = m_sprBgColl;
            m_sprBgColl = 0;
            break;
        // Color registers: only low 4 bits exist; high bits read as 1
        case EXTCOL: case BGCOL0: case BGCOL1: case BGCOL2: case BGCOL3:
        case SPMC0:  case SPMC1:
        case SP0COL: case SP1COL: case SP2COL: case SP3COL:
        case SP4COL: case SP5COL: case SP6COL: case SP7COL:
            *val = m_regs[reg] | 0xF0;
            break;
        default:
            *val = m_regs[reg];
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// IOHandler: ioWrite
// ---------------------------------------------------------------------------

bool VIC2::ioWrite(IBus* /*bus*/, uint32_t addr, uint8_t val) {
    if ((addr & ~addrMask()) != m_baseAddr) return false;

    uint8_t reg = addr & 0x3F;
    if (reg > 0x2E) return true; // writes to unused regs are ignored

    switch (reg) {
        case IRQ:
            // Writing 1 to a bit clears that IRQ flag.
            m_regs[IRQ] &= ~(val & 0x0F);
            updateIrq();
            break;
        case IRQEN:
            m_regs[IRQEN] = val & 0x0F;
            updateIrq();
            break;
        case SSCOL:
        case SBCOL:
            // Read-only collision registers; writes are ignored.
            break;
        default:
            m_regs[reg] = val;
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// IOHandler: tick — drives the raster counter and fires raster IRQ
// ---------------------------------------------------------------------------

void VIC2::tick(uint64_t cycles) {
    m_cycleAccum += cycles;

    // Advance raster lines based on accumulated cycles.
    while (m_cycleAccum >= (uint64_t)CYCLES_PER_LINE) {
        m_cycleAccum -= CYCLES_PER_LINE;
        m_rasterLine++;

        if (m_rasterLine >= LINES_PER_FRAME) {
            m_rasterLine = 0;
        }

        static int tickLogCounter = 0;
        if (tickLogCounter++ % 10000 == 0) {
            char msg[128];
            std::snprintf(msg, sizeof(msg), "VIC2::tick: raster=%d cycleAccum=%lu", m_rasterLine, m_cycleAccum);
            log(SIM_LOG_DEBUG, msg);
        }

        // Raster compare: fires when current line matches $D011 bit-8 + $D012.
        uint16_t compare = (uint16_t)(m_regs[RASTER]) |
                           ((m_regs[SCR1] & SCR1_RASTER8) ? 0x100 : 0);
        if (m_rasterLine == compare) {
            m_regs[IRQ] |= IRQ_RST;
            updateIrq();
        }
    }
}

// ---------------------------------------------------------------------------
// IVideoOutput: renderFrame
// ---------------------------------------------------------------------------

void VIC2::renderFrame(uint32_t* buf) {
    if (!buf) return;

    uint32_t borderCol = palette(m_regs[EXTCOL] & 0x0F);

    // Fill the entire frame with the border colour first.
    for (int i = 0; i < FRAME_W * FRAME_H; ++i)
        buf[i] = borderCol;

    // Render the background (text/bitmap) into the 320×200 display area.
    renderBackground(buf);

    // Render sprites on top.
    renderSprites(buf);
}

// ---------------------------------------------------------------------------
// Background rendering — five video modes
// ---------------------------------------------------------------------------

void VIC2::renderBackground(uint32_t* buf) {
    if (!(m_regs[SCR1] & SCR1_DEN)) return; // display disabled

    bool ecm = (m_regs[SCR1] & SCR1_ECM) != 0;
    bool bmm = (m_regs[SCR1] & SCR1_BMM) != 0;
    bool mcm = (m_regs[SCR2] & SCR2_MCM) != 0;

    uint32_t scrBase  = screenBase();
    uint32_t cbBase   = charBitmapBase();

    // On C64, character generator ROM is always seen by the VIC at bank 
    // offsets $1000-$1FFF and $9000-$9FFF BUT only in Banks 0 and 2.
    bool isBank0or2 = (m_bankBase == 0x0000 || m_bankBase == 0x8000);
    bool useCharRom = !bmm && isBank0or2 && (cbBase == 0x1000);

    // Logging for debug
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        char msg[256];
        std::snprintf(msg, sizeof(msg), 
            "renderBackground: ECM=%d BMM=%d MCM=%d SCRBASE=$%04X CBBASE=$%04X BANK=$%04X CHARROM=%d ROMPTR=%p",
            (int)ecm, (int)bmm, (int)mcm, scrBase, cbBase, m_bankBase, (int)useCharRom, (void*)m_charRom);
        log(SIM_LOG_DEBUG, msg);
        
        // Sample first few bytes of screen RAM
        uint8_t s0 = dmaPeek(0 + (scrBase & 0x3FFF));
        uint8_t s1 = dmaPeek(1 + (scrBase & 0x3FFF));
        uint8_t c0 = m_colorRam ? (m_colorRam[0] & 0x0F) : 0;
        std::snprintf(msg, sizeof(msg), "Sample: SCR[0]=$%02X SCR[1]=$%02X COL[0]=$%02X", s0, s1, c0);
        log(SIM_LOG_DEBUG, msg);

        // Sample screen content: count non-space characters
        int nonSpace = 0;
        for (int i = 0; i < 1000; ++i) {
            if (dmaPeek(scrBase + i) != 0x20) nonSpace++;
        }
        std::snprintf(msg, sizeof(msg), "Screen Content: %d / 1000 cells are not space ($20)", nonSpace);
        log(SIM_LOG_DEBUG, msg);

        // Sample character ROM data for character $20 (space) and $01 (A)
        if (m_charRom) {
            uint8_t b20 = charRomByte(0x20 * 8); // first byte of space
            uint8_t b01 = charRomByte(0x01 * 8); // first byte of 'A'
            uint8_t b2A = charRomByte(0x2A * 8); // first byte of '*'
            std::snprintf(msg, sizeof(msg), "CharROM Sample: CHAR[$20][0]=$%02X CHAR[$01][0]=$%02X CHAR[$2A][0]=$%02X", b20, b01, b2A);
            log(SIM_LOG_DEBUG, msg);
        } else {
            log(SIM_LOG_DEBUG, "CharROM is NULL!");
        }
    }

    uint32_t bg0 = palette(m_regs[BGCOL0] & 0x0F);
    uint32_t bg1 = palette(m_regs[BGCOL1] & 0x0F);
    uint32_t bg2 = palette(m_regs[BGCOL2] & 0x0F);
    uint32_t bg3 = palette(m_regs[BGCOL3] & 0x0F);

    // Color RAM is a separate 4-bit RAM on real C64 hardware, accessed via a
    // dedicated connection to the VIC-II (not through the DMA bus).  The
    // machine factory calls setColorRam() to supply the pointer directly.

    for (int row = 0; row < 25; ++row) {
        for (int col = 0; col < 40; ++col) {
            int cellIdx = row * 40 + col;
            uint8_t code = dmaPeek(scrBase + cellIdx);

            int px = DISPLAY_X + col * 8;
            int py = DISPLAY_Y + row * 8;

            if (bmm && !ecm) {
                // -------------------------------------------------------
                // Bitmap modes (Standard or Multicolor)
                // -------------------------------------------------------
                uint32_t bitmapOffset = cbBase + cellIdx * 8;
                uint8_t  colorByte    = code; // screen RAM holds colors in bitmap mode

                if (!mcm) {
                    // Standard bitmap: 2 colors per 8×8 cell
                    uint8_t fgIdx = (colorByte >> 4) & 0x0F;
                    uint8_t bgIdx = colorByte & 0x0F;
                    uint32_t fgCol = palette(fgIdx);
                    uint32_t bgCol = palette(bgIdx);

                    for (int r = 0; r < 8; ++r) {
                        uint8_t bits = dmaPeek(bitmapOffset + r);
                        int fy = py + r;
                        if (fy < DISPLAY_Y || fy >= DISPLAY_Y + DISPLAY_H) continue;
                        for (int b = 7; b >= 0; --b) {
                            int fx = px + (7 - b);
                            if (fx < DISPLAY_X || fx >= DISPLAY_X + DISPLAY_W) continue;
                            buf[fy * FRAME_W + fx] = (bits >> b) & 1 ? fgCol : bgCol;
                        }
                    }
                } else {
                    // Multicolor bitmap: 4 colors per 8×8 cell, 4×8 effective resolution
                    uint8_t c00 = m_regs[BGCOL0] & 0x0F;
                    uint8_t c01 = (colorByte >> 4) & 0x0F;
                    uint8_t c10 = colorByte & 0x0F;
                    // Color RAM: direct 4-bit connection.
                    uint8_t c11 = m_colorRam ? (m_colorRam[cellIdx & 0x3FF] & 0x0F) : 0;

                    uint32_t mc[4] = { palette(c00), palette(c01), palette(c10), palette(c11) };

                    for (int r = 0; r < 8; ++r) {
                        uint8_t bits = dmaPeek(bitmapOffset + r);
                        int fy = py + r;
                        if (fy < DISPLAY_Y || fy >= DISPLAY_Y + DISPLAY_H) continue;
                        for (int pair = 0; pair < 4; ++pair) {
                            uint8_t sel = (bits >> (6 - pair * 2)) & 0x03;
                            uint32_t c = mc[sel];
                            int fx = px + pair * 2;
                            if (fx >= DISPLAY_X && fx + 1 < DISPLAY_X + DISPLAY_W) {
                                buf[fy * FRAME_W + fx]     = c;
                                buf[fy * FRAME_W + fx + 1] = c;
                            }
                        }
                    }
                }

            } else {
                // -------------------------------------------------------
                // Text modes (Standard, Multicolor, Extended background)
                // -------------------------------------------------------
                uint8_t charCode;
                uint32_t bgPx;

                if (ecm) {
                    // Extended background: char code is low 6 bits; top 2 bits select BG
                    uint8_t bgSel = (code >> 6) & 0x03;
                    charCode = code & 0x3F;
                    uint32_t bgs[4] = { bg0, bg1, bg2, bg3 };
                    bgPx = bgs[bgSel];
                } else {
                    charCode = code;
                    bgPx = bg0;
                }

                // Color RAM: direct 4-bit connection, not via DMA bus.
                uint8_t colorNibble = m_colorRam ? (m_colorRam[cellIdx & 0x3FF] & 0x0F) : 0x0F;

                uint32_t glyphOffset = cbBase + charCode * 8;

                for (int r = 0; r < 8; ++r) {
                    uint8_t bits;
                    if (useCharRom) {
                        bits = charRomByte(charCode * 8 + r);
                    } else {
                        bits = dmaPeek(glyphOffset + r);
                    }

                    int fy = py + r;
                    if (fy < DISPLAY_Y || fy >= DISPLAY_Y + DISPLAY_H) continue;

                    if (!mcm) {
                        // Standard text: 1 bit per pixel, fg/bg
                        uint32_t fgCol = palette(colorNibble);
                        for (int b = 7; b >= 0; --b) {
                            int fx = px + (7 - b);
                            if (fx < DISPLAY_X || fx >= DISPLAY_X + DISPLAY_W) continue;
                            buf[fy * FRAME_W + fx] = (bits >> b) & 1 ? fgCol : bgPx;
                        }
                    } else {
                        // Multicolor text: high bit of color nibble selects mode
                        if (colorNibble & 0x08) {
                            // Multicolor: 2 bits per pixel → 4 colors, 4×8 resolution
                            uint32_t mc[4] = {
                                bgPx,
                                palette(m_regs[BGCOL1] & 0x0F),
                                palette(m_regs[BGCOL2] & 0x0F),
                                palette(colorNibble & 0x07)
                            };
                            for (int pair = 0; pair < 4; ++pair) {
                                uint8_t sel = (bits >> (6 - pair * 2)) & 0x03;
                                int fx = px + pair * 2;
                                if (fx >= DISPLAY_X && fx + 1 < DISPLAY_X + DISPLAY_W) {
                                    buf[fy * FRAME_W + fx]     = mc[sel];
                                    buf[fy * FRAME_W + fx + 1] = mc[sel];
                                }
                            }
                        } else {
                            // High-res half of multicolor: treated as standard text
                            uint32_t fgCol = palette(colorNibble);
                            for (int b = 7; b >= 0; --b) {
                                int fx = px + (7 - b);
                                if (fx < DISPLAY_X || fx >= DISPLAY_X + DISPLAY_W) continue;
                                buf[fy * FRAME_W + fx] = (bits >> b) & 1 ? fgCol : bgPx;
                            }
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Sprite rendering
// ---------------------------------------------------------------------------

void VIC2::renderSprites(uint32_t* buf) {
    uint32_t scrBase = screenBase();

    // Render sprites in reverse priority order (sprite 7 first, 0 last → 0 on top)
    for (int sp = 7; sp >= 0; --sp) {
        if (!(m_regs[SPENA] & (1 << sp))) continue; // sprite disabled

        // Sprite position
        uint16_t spX = (uint16_t)m_regs[SP0X + sp * 2];
        if (m_regs[MSIGX] & (1 << sp)) spX |= 0x100;
        uint8_t  spY = m_regs[SP0Y + sp * 2];

        bool xExp = (m_regs[XXPAND] & (1 << sp)) != 0;
        bool yExp = (m_regs[YXPAND] & (1 << sp)) != 0;
        bool mc   = (m_regs[SPMC]   & (1 << sp)) != 0;
        bool prio = (m_regs[SPBGPR] & (1 << sp)) != 0; // 1 = behind background

        // Sprite data pointer: last 8 bytes of the 1024-byte screen matrix area
        uint8_t  ptr       = dmaPeek(scrBase + 1016 + sp);
        uint32_t dataBase  = (uint32_t)ptr * 64; // sprite data offset within VIC bank

        uint32_t spColor   = palette(m_regs[SP0COL + sp] & 0x0F);
        uint32_t spMc0     = palette(m_regs[SPMC0]  & 0x0F);
        uint32_t spMc1     = palette(m_regs[SPMC1]  & 0x0F);

        int sprH = yExp ? 42 : 21;
        int sprW = xExp ? 48 : 24;

        for (int r = 0; r < sprH; ++r) {
            // Which source row? Y expansion doubles each source row.
            int srcRow = yExp ? r / 2 : r;
            if (srcRow >= 21) break;

            int fy = (int)spY + r - 50; // C64 sprite Y origin offset ≈ 50 (top of display)
            int screenY = fy + DISPLAY_Y - 50;
            if (screenY < 0 || screenY >= FRAME_H) continue;

            // 3 bytes per sprite row = 24 pixels
            uint8_t b0 = dmaPeek(dataBase + srcRow * 3 + 0);
            uint8_t b1 = dmaPeek(dataBase + srcRow * 3 + 1);
            uint8_t b2 = dmaPeek(dataBase + srcRow * 3 + 2);
            uint32_t rowBits = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;

            for (int c = 0; c < sprW; ++c) {
                int srcPx = xExp ? c / 2 : c;

                int fx = (int)spX + c - 24; // C64 sprite X origin offset ≈ 24
                int screenX = fx; // X is relative to the total frame
                if (screenX < 0 || screenX >= FRAME_W) continue;

                uint32_t pxColor;
                if (!mc) {
                    // Monochrome: 1 bit per pixel
                    uint8_t bit = (rowBits >> (23 - srcPx)) & 1;
                    if (!bit) continue; // transparent
                    pxColor = spColor;
                } else {
                    // Multicolor: 2 bits per pixel (only even srcPx advances color pair)
                    int pair = srcPx / 2;
                    uint8_t sel = (rowBits >> (22 - pair * 2)) & 0x03;
                    if (sel == 0) continue; // transparent
                    if      (sel == 1) pxColor = spMc0;
                    else if (sel == 2) pxColor = spColor;
                    else               pxColor = spMc1;
                }

                // Priority: if sprite is behind background, only draw over border/BG0
                if (prio) {
                    uint32_t existing = buf[screenY * FRAME_W + screenX];
                    uint32_t borderCol = palette(m_regs[EXTCOL] & 0x0F);
                    uint32_t bg0Col    = palette(m_regs[BGCOL0] & 0x0F);
                    if (existing != borderCol && existing != bg0Col) continue;
                }

                // Sprite-background collision: if a non-transparent pixel would
                // overwrite a non-border display pixel.
                {
                    uint32_t existing = buf[screenY * FRAME_W + screenX];
                    uint32_t borderCol = palette(m_regs[EXTCOL] & 0x0F);
                    if (existing != borderCol &&
                        screenX >= DISPLAY_X && screenX < DISPLAY_X + DISPLAY_W &&
                        screenY >= DISPLAY_Y && screenY < DISPLAY_Y + DISPLAY_H) {
                        m_sprBgColl |= (1 << sp);
                    }
                }

                buf[screenY * FRAME_W + screenX] = pxColor;
            }
        }
    }

    // Sprite-sprite collision: detect overlapping sprite pixels.
    // Simplified: mark pairs by checking if two enabled sprites overlap in bounding box.
    for (int a = 0; a < 8; ++a) {
        if (!(m_regs[SPENA] & (1 << a))) continue;
        for (int b = a + 1; b < 8; ++b) {
            if (!(m_regs[SPENA] & (1 << b))) continue;
            uint16_t ax = m_regs[SP0X + a*2] | ((m_regs[MSIGX] & (1<<a)) ? 0x100 : 0);
            uint16_t bx = m_regs[SP0X + b*2] | ((m_regs[MSIGX] & (1<<b)) ? 0x100 : 0);
            uint8_t  ay = m_regs[SP0Y + a*2];
            uint8_t  by = m_regs[SP0Y + b*2];
            int aw = (m_regs[XXPAND] & (1<<a)) ? 48 : 24;
            int bw = (m_regs[XXPAND] & (1<<b)) ? 48 : 24;
            int ah = (m_regs[YXPAND] & (1<<a)) ? 42 : 21;
            int bh = (m_regs[YXPAND] & (1<<b)) ? 42 : 21;
            // Bounding box overlap test
            if ((int)ax < (int)bx + bw && (int)ax + aw > (int)bx &&
                (int)ay < (int)by + bh && (int)ay + ah > (int)by) {
                m_sprSprColl |= (1 << a) | (1 << b);
            }
        }
    }

    // Fire collision IRQs if enabled and collisions occurred.
    if (m_sprSprColl) { m_regs[IRQ] |= IRQ_MMC; updateIrq(); }
    if (m_sprBgColl)  { m_regs[IRQ] |= IRQ_MBC; updateIrq(); }
}

// ---------------------------------------------------------------------------
// IRQ helper
// ---------------------------------------------------------------------------

void VIC2::updateIrq() {
    bool assert = (m_regs[IRQ] & m_regs[IRQEN] & 0x0F) != 0;
    if (m_irqLine) m_irqLine->set(assert);
}
