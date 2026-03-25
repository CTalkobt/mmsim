#pragma once

/**
 * Interface for a single binary signal line (e.g. IRQ, NMI, Reset, or control pins).
 */
class ISignalLine {
public:
    virtual ~ISignalLine() {}

    /**
     * Get the current logic level of the line.
     */
    virtual bool get() const = 0;

    /**
     * Set the logic level of the line.
     */
    virtual void set(bool level) = 0;

    /**
     * Trigger a short pulse (toggle then return to previous state, or just trigger).
     */
    virtual void pulse() = 0;
};
