#pragma once

#include <cstdint>
#include "libcore/icore.h"
#include "libmem/ibus.h"
#include "libtoolchain/idisasm.h"

/**
 * Interface for observing CPU execution and memory events.
 */
class ExecutionObserver {
public:
    virtual ~ExecutionObserver() {}

    /**
     * Called after each instruction step.
     */
    virtual void onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) {
        (void)cpu; (void)bus; (void)entry;
    }

    /**
     * Called whenever a byte is written to memory.
     */
    virtual void onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) {
        (void)bus; (void)addr; (void)before; (void)after;
    }

    /**
     * Called whenever a byte is read from memory.
     */
    virtual void onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) {
        (void)bus; (void)addr; (void)val;
    }
};
