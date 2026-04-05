#pragma once

#include <cstdint>

class IBus;
class IVideoOutput;

/**
 * Abstract base class for memory-mapped I/O devices.
 */
class IOHandler {
public:
    enum class InterfaceID {
        VideoOutput,
        AudioOutput,
    };

    virtual ~IOHandler();

    /**
     * Unique name of the device (e.g. "VIC-II").
     */
    virtual const char* name() const = 0;

    /**
     * Base address where this device is mapped.
     */
    virtual uint32_t baseAddr() const = 0;

    /**
     * Address mask to determine the range handled by this device.
     */
    virtual uint32_t addrMask() const = 0;

    /**
     * Read from the device at the given address.
     * @return true if the address was handled by this device.
     */
    virtual bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) = 0;

    /**
     * Write to the device at the given address.
     * @return true if the address was handled by this device.
     */
    virtual bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) = 0;

    /**
     * Reset the device to its power-on state.
     */
    virtual void reset() = 0;

    /**
     * Tick the device forward. Called each CPU step or cycle.
     */
    virtual void tick(uint64_t cycles) = 0;

    /**
     * Get a pointer to a specific interface implemented by this device.
     * @param id The ID of the interface to get.
     * @return A pointer to the interface, or nullptr if not implemented.
     */
    virtual void* getInterface(InterfaceID id) { return nullptr; }
};
