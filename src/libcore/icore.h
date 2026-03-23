#pragma once

#include <cstdint>
#include <cstddef>
#include "libmem/ibus.h"

/**
 * Register width.
 */
enum class RegWidth {
    R8 = 1,
    R16 = 2,
    R32 = 4
};

/**
 * Register flags.
 */
#define REGFLAG_PC          0x01  // Program counter
#define REGFLAG_SP          0x02  // Stack pointer
#define REGFLAG_STATUS      0x04  // Flags / status register
#define REGFLAG_ACCUMULATOR 0x08  // Primary accumulator
#define REGFLAG_INTERNAL    0x10  // Hidden from UI
#define REGFLAG_SHADOW      0x20  // Alternate/shadow register

/**
 * ISA capabilities bitmask.
 */
#define ISA_CAP_HARVARD       0x0001
#define ISA_CAP_IO_PORT_SPACE 0x0002
#define ISA_CAP_32BIT_ADDR    0x0004
#define ISA_CAP_MULTI_PREFIX  0x0008
#define ISA_CAP_16BIT_DATA    0x0010
#define ISA_CAP_THUMB_MODE    0x0020

/**
 * Register descriptor.
 */
struct RegDescriptor {
    const char* name;
    RegWidth    width;
    uint32_t    flags;
    const char* alias;      // e.g., "F" for 6502 "P"
};

class ExecutionObserver;

/**
 * Abstract interface for a CPU core.
 */
class ICore {
public:
    virtual ~ICore() {}

    // ISA identification
    virtual const char* isaName()     const = 0;
    virtual const char* variantName() const = 0;
    virtual uint32_t    isaCaps()     const = 0;

    // Register descriptor table
    virtual int                  regCount()             const = 0;
    virtual const RegDescriptor* regDescriptor(int idx) const = 0;
    virtual uint32_t             regRead (int idx)      const = 0;
    virtual void                 regWrite(int idx, uint32_t val) = 0;

    // Named register helpers
    virtual uint32_t regReadByName (const char* name) const;
    virtual void     regWriteByName(const char* name, uint32_t val);
    virtual int      regIndexByName(const char* name) const;

    // Convenience accessors
    virtual uint32_t pc() const = 0;
    virtual void     setPc(uint32_t addr) = 0;
    virtual uint32_t sp() const = 0;

    // Execution
    virtual int  step()  = 0;   // Returns cycle count
    virtual void reset() = 0;

    // Interrupts
    virtual void triggerIrq()   = 0;
    virtual void triggerNmi()   = 0;
    virtual void triggerReset() = 0;

    // Disassembly
    virtual int disassembleOne  (IBus* bus, uint32_t addr, char* buf, int bufsz) = 0;
    virtual int disassembleEntry(IBus* bus, uint32_t addr, void* entryOut)      = 0;

    // Snapshot support
    virtual size_t stateSize()             const = 0;
    virtual void   saveState(uint8_t* buf) const = 0;
    virtual void   loadState(const uint8_t* buf) = 0;

    // Step-over / step-out heuristics
    virtual int  isCallAt    (IBus* bus, uint32_t addr) = 0;
    virtual bool isReturnAt  (IBus* bus, uint32_t addr) = 0;
    virtual bool isProgramEnd(IBus* bus)                = 0;

    // Helper to write a call harness
    virtual int writeCallHarness(IBus* bus, uint32_t scratch, uint32_t routineAddr) = 0;

    // Bus attachment
    virtual void setDataBus(IBus* bus) = 0;
    virtual void setCodeBus(IBus* bus) = 0;
    virtual void setIoBus  (IBus* bus) = 0;

    virtual uint64_t cycles() const = 0;

    ExecutionObserver* observer = nullptr;
};
