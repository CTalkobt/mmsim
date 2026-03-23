#pragma once

#include <cstdint>
#include <cstddef>

/**
 * Role of a specific bus in the system.
 */
enum class BusRole {
    DATA,       // Main memory or data bus
    CODE,       // Instruction bus (for Harvard architectures)
    IO_PORT,    // Separate I/O space (e.g., Z80 IN/OUT)
    DMA         // Dedicated DMA view
};

/**
 * Configuration for an address bus.
 */
struct BusConfig {
    uint32_t addrBits;      // 16, 24, 28, 32
    uint32_t dataBits;      // 8, 16, 32
    BusRole  role;
    bool     littleEndian;
    uint32_t addrMask;      // (1u << addrBits) - 1
};

/**
 * Abstract interface for an address / memory bus.
 */
class IBus {
public:
    virtual ~IBus() {}

    virtual const BusConfig& config() const = 0;
    virtual const char*      name()   const = 0;

    /**
     * Primary 8-bit access.
     */
    virtual uint8_t  read8 (uint32_t addr) = 0;
    virtual void     write8(uint32_t addr, uint8_t val) = 0;

    /**
     * Multi-byte helpers. Default implementation assembles from read8/write8.
     */
    virtual uint16_t read16 (uint32_t addr);
    virtual void     write16(uint32_t addr, uint16_t val);
    virtual uint32_t read32 (uint32_t addr);
    virtual void     write32(uint32_t addr, uint32_t val);

    /**
     * Side-effect-free read for debugger/disassembler.
     * Default implementation delegates to read8.
     */
    virtual uint8_t peek8(uint32_t addr) { return read8(addr); }

    virtual void reset() {}

    /**
     * Snapshot support.
     */
    virtual size_t stateSize()             const { return 0; }
    virtual void   saveState(uint8_t *buf) const { (void)buf; }
    virtual void   loadState(const uint8_t *buf) { (void)buf; }

    /**
     * Write-log for debug snapshot/diff.
     */
    virtual int  writeCount()                                               const { return 0; }
    virtual void getWrites(uint32_t *addrs, uint8_t *before,
                            uint8_t *after, int max)                        const {}
    virtual void clearWriteLog() {}
};
