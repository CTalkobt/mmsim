#pragma once

#include <vector>
#include <cstdint>
#include "io_handler.h"

class IBus;

/**
 * Registry and dispatcher for multiple IOHandler devices.
 */
class IORegistry {
public:
    IORegistry() = default;
    ~IORegistry() = default;

    /**
     * Register an I/O handler. The registry does NOT own the pointer.
     */
    void registerHandler(IOHandler* handler);

    /**
     * Dispatch a read to the first handler that matches the address.
     */
    bool dispatchRead(IBus* bus, uint32_t addr, uint8_t* val);

    /**
     * Dispatch a write to the first handler that matches the address.
     */
    bool dispatchWrite(IBus* bus, uint32_t addr, uint8_t val);

    /**
     * Call tick() on all registered handlers.
     */
    void tickAll(uint64_t cycles);

    /**
     * Call reset() on all registered handlers.
     */
    void resetAll();

    /**
     * Enumerate all registered handlers.
     */
    void enumerate(std::vector<IOHandler*>& list);

    /**
     * Remove all handlers from the registry.
     */
    void clear();

private:
    std::vector<IOHandler*> m_handlers;
};
