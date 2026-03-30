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
     * Note: For shared lines (like IRQ), callers should avoid calling set(false) 
     * if their internal state has not changed, to avoid overwriting an active 
     * interrupt signal from another device sharing the same line.
     */
    virtual void set(bool level) = 0;

    /**
     * Trigger a short pulse (toggle then return to previous state, or just trigger).
     */
    virtual void pulse() = 0;
};
