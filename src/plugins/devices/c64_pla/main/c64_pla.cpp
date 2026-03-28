#include "c64_pla.h"

bool C64PLA::ioRead(IBus* /*bus*/, uint32_t addr, uint8_t* val) {
    // -----------------------------------------------------------------------
    // KERNAL ROM: $E000–$FFFF  active when HIRAM=1
    // -----------------------------------------------------------------------
    if (addr >= 0xE000 && addr <= 0xFFFF) {
        if (hiram() && m_kernalRom) {
            uint32_t off = addr - 0xE000;
            *val = (off < m_kernalSize) ? m_kernalRom[off] : 0xFF;
            return true;
        }
        return false; // flat RAM
    }

    // -----------------------------------------------------------------------
    // $D000–$DFFF: Char ROM (CHAREN=0) or I/O (CHAREN=1).
    // Both require HIRAM=1; when HIRAM=0 the area is plain RAM.
    // -----------------------------------------------------------------------
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        if (hiram()) {
            if (!charen() && m_charRom) {
                // Character ROM: 4 KB, $D000–$DFFF (the 4 KB image repeats).
                uint32_t off = (addr - 0xD000) & (m_charSize - 1);
                *val = m_charRom[off];
                return true;
            }
            // I/O mode (CHAREN=1): return false so the IORegistry dispatches
            // to the VIC-II / SID / CIA / color-RAM handlers next.
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // BASIC ROM: $A000–$BFFF  active when HIRAM=1 && LORAM=1
    // -----------------------------------------------------------------------
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (hiram() && loram() && m_basicRom) {
            uint32_t off = addr - 0xA000;
            *val = (off < m_basicSize) ? m_basicRom[off] : 0xFF;
            return true;
        }
        return false; // flat RAM
    }

    return false;
}

bool C64PLA::ioWrite(IBus* /*bus*/, uint32_t /*addr*/, uint8_t /*val*/) {
    // All writes fall through to the underlying flat RAM or to the I/O device
    // handlers (CIA, VIC-II, SID) that follow the PLA in the dispatch chain.
    // ROM areas accept writes to the underlying RAM; the PLA only shadows reads.
    return false;
}
