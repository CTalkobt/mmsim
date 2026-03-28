#pragma once

#include "libdevices/main/io_handler.h"
#include "libdevices/main/isignal_line.h"

/**
 * C64 PLA (Programmable Logic Array) — Banking Controller.
 *
 * Monitors the three 6510 I/O port signal lines (LORAM, HIRAM, CHAREN) and
 * decides which of the following is visible at each banking region on every
 * memory read:
 *
 *   $A000–$BFFF  BASIC ROM    when HIRAM=1 && LORAM=1;  else flat RAM
 *   $D000–$DFFF  Char ROM     when HIRAM=1 && CHAREN=0; else I/O or flat RAM
 *   $D000–$DFFF  I/O devices  when HIRAM=1 && CHAREN=1  (other handlers respond)
 *   $E000–$FFFF  KERNAL ROM   when HIRAM=1;              else flat RAM
 *
 * Writes always fall through to the underlying flat RAM (ROM areas are
 * read-only by hardware convention, not by bus protection) or to the I/O
 * device handlers registered in the IORegistry.
 *
 * The PLA registers itself in the IORegistry with baseAddr 0xA000 so the
 * sort-by-base-address ordering guarantees it is called before the I/O device
 * handlers at $D000–$DFFF. When it returns false the IORegistry continues to
 * the next handler (CIA, VIC-II, SID, etc.), which is the correct behaviour
 * for the I/O-visible state.
 *
 * Usage (machine factory):
 *   auto* pla = new C64PLA();
 *   pla->setSignals(cpu6510->signalLoram(),
 *                   cpu6510->signalHiram(),
 *                   cpu6510->signalCharen());
 *   pla->setBasicRom (basicBuf,  8192);
 *   pla->setKernalRom(kernalBuf, 8192);
 *   pla->setCharRom  (charBuf,   4096);
 *   ioRegistry->registerHandler(pla);
 */
class C64PLA : public IOHandler {
public:
    C64PLA() = default;
    ~C64PLA() override = default;

    // -----------------------------------------------------------------------
    // Configuration (call before registering in IORegistry)
    // -----------------------------------------------------------------------

    void setSignals(ISignalLine* loram, ISignalLine* hiram, ISignalLine* charen) {
        m_loram  = loram;
        m_hiram  = hiram;
        m_charen = charen;
    }

    void setBasicRom (const uint8_t* data, uint32_t size) { m_basicRom  = data; m_basicSize  = size; }
    void setKernalRom(const uint8_t* data, uint32_t size) { m_kernalRom = data; m_kernalSize = size; }
    void setCharRom  (const uint8_t* data, uint32_t size) { m_charRom   = data; m_charSize   = size; }

    // -----------------------------------------------------------------------
    // IOHandler interface
    // -----------------------------------------------------------------------

    const char* name()     const override { return "C64PLA"; }
    uint32_t    baseAddr() const override { return 0xA000; }  // sort before $D000 devices
    uint32_t    addrMask() const override { return 0xFFFF; }  // range checked internally

    bool ioRead (IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t  val) override;
    void reset() override {}
    void tick(uint64_t) override {}

private:
    ISignalLine* m_loram  = nullptr;
    ISignalLine* m_hiram  = nullptr;
    ISignalLine* m_charen = nullptr;

    const uint8_t* m_basicRom  = nullptr;  uint32_t m_basicSize  = 0;
    const uint8_t* m_kernalRom = nullptr;  uint32_t m_kernalSize = 0;
    const uint8_t* m_charRom   = nullptr;  uint32_t m_charSize   = 0;

    bool hiram()  const { return m_hiram  ? m_hiram->get()  : true; }
    bool loram()  const { return m_loram  ? m_loram->get()  : true; }
    bool charen() const { return m_charen ? m_charen->get() : true; }
};
