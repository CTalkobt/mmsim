#pragma once

#include <cstdint>
#include <string>

/**
 * Interface for matrix keyboard devices.
 * Represents any keyboard implemented as an N×N switch matrix wired to two I/O ports
 * (one for column drive, one for row sense), as found in most 8-bit home computers.
 */
class IKeyboardMatrix {
public:
    virtual ~IKeyboardMatrix() {}

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
     * Press or release a key by symbolic name (e.g. "a", "return", "f1").
     * @return true if the key was found.
     */
    virtual bool pressKeyByName(const std::string& keyName, bool down) = 0;

    /**
     * Retrieve the raw I/O port for this matrix.
     * Index 0 is the column/output port; Index 1 is the row/input port.
     */
    virtual class IPortDevice* getPort(int index) = 0;
};
