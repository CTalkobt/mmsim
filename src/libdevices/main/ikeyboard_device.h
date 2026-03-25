#pragma once

#include <cstdint>
#include <string>

/**
 * Interface for keyboard devices.
 */
class IKeyboardDevice {
public:
    virtual ~IKeyboardDevice() {}

    /**
     * Press a key.
     * @param row Matrix row (0-7).
     * @param col Matrix column (0-7).
     */
    virtual void keyDown(int row, int col) = 0;

    /**
     * Release a key.
     */
    virtual void keyUp(int row, int col) = 0;

    /**
     * Clear all pressed keys.
     */
    virtual void clearKeys() = 0;

    /**
     * Map a platform-specific keycode to a matrix position and press it.
     * @param keyName Symbolic name of the key (e.g. "A", "RETURN", "F1").
     * @return true if the key was found and pressed.
     */
    virtual bool pressKeyByName(const std::string& keyName, bool down) = 0;

    /**
     * Retrieve the raw I/O ports for this keyboard (if applicable).
     * Index 0 is typically the column/output port, Index 1 is the row/input port.
     */
    virtual class IPortDevice* getPort(int index) = 0;
};
