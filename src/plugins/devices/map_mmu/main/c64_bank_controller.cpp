#include "c64_bank_controller.h"
#include "map_mmu.h"
#include "libmem/main/sparse_memory_bus.h"

C64BankController::C64BankController(SparseMemoryBus* physBus)
    : m_physBus(physBus)
{
}

void C64BankController::setBasicRom(const uint8_t* data, uint32_t size) {
    m_basicRom  = data;
    m_basicSize = size;
    updateOverlays();
}

void C64BankController::setKernalRom(const uint8_t* data, uint32_t size) {
    m_kernalRom  = data;
    m_kernalSize = size;
    updateOverlays();
}

void C64BankController::setCharRom(const uint8_t* data, uint32_t size) {
    m_charRom  = data;
    m_charSize = size;
    // Char ROM overlay at $D000 is handled via ioRead, not SparseMemoryBus overlays,
    // because it conflicts with I/O device handlers in the same region.
}

void C64BankController::reset() {
    m_portDDR = 0x2F;
    m_portOut = 0x37;  // LORAM=1, HIRAM=1, CHAREN=1 — KERNAL+BASIC visible, I/O active
    updateOverlays();
}

bool C64BankController::isBlockMapped(int block) const {
    if (!m_mapMmu) return false;
    return m_mapMmu->getMapState().enables & (1 << block);
}

void C64BankController::updateOverlays() {
    // BASIC ROM: $A000-$BFFF (blocks 5) — visible when HIRAM=1 && LORAM=1 && block not MAP'd
    if (hiram() && loram() && m_basicRom && !isBlockMapped(5)) {
        m_physBus->addRomOverlay(0x00A000, m_basicSize, m_basicRom);
    } else {
        m_physBus->removeRomOverlay(0x00A000);
    }

    // KERNAL ROM: $E000-$FFFF (block 7) — visible when HIRAM=1 && block not MAP'd
    if (hiram() && m_kernalRom && !isBlockMapped(7)) {
        m_physBus->addRomOverlay(0x00E000, m_kernalSize, m_kernalRom);
    } else {
        m_physBus->removeRomOverlay(0x00E000);
    }

    // Note: Char ROM at $D000 is NOT managed via overlays because it conflicts
    // with I/O handlers. It's served via ioRead() when CHAREN=0.
}

bool C64BankController::ioRead(IBus* /*bus*/, uint32_t addr, uint8_t* val) {
    // CPU I/O port: $00 (DDR), $01 (port)
    if (addr == 0x0000) {
        *val = m_portDDR;
        return true;
    }
    if (addr == 0x0001) {
        // Output bits come from latch, input bits float high
        *val = effectivePort();
        return true;
    }

    // Character ROM: $D000-$DFFF when HIRAM=1 && CHAREN=0 && block 6 not MAP'd
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        if (hiram() && !charen() && m_charRom && !isBlockMapped(6)) {
            uint32_t off = (addr - 0xD000) & (m_charSize - 1);
            *val = m_charRom[off];
            return true;
        }
        // CHAREN=1 or HIRAM=0: fall through to I/O handlers or RAM
    }

    return false;
}

bool C64BankController::ioWrite(IBus* /*bus*/, uint32_t addr, uint8_t val) {
    // CPU I/O port: $00 (DDR), $01 (port)
    if (addr == 0x0000) {
        m_portDDR = val;
        updateOverlays();
        return true;
    }
    if (addr == 0x0001) {
        m_portOut = val;
        updateOverlays();
        return true;
    }

    // All other writes fall through (ROM writes go to underlying RAM,
    // I/O writes go to device handlers)
    return false;
}
