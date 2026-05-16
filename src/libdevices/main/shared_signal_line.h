#pragma once

#include "isignal_line.h"
#include <string>
#include <vector>
#include <algorithm>

/**
 * A shared signal line (like IRQ or NMI) where multiple sources can pull the line low (or set it high).
 * Uses a simple OR logic (any source high -> line high) or active-low logic (any source low -> line low).
 * In this implementation, we use simple boolean state.
 */
class SharedSignalLine : public ISignalLine {
public:
    explicit SharedSignalLine(const std::string& name) : m_name(name), m_level(false) {}
    ~SharedSignalLine() override = default;

    const char* name() const { return m_name.c_str(); }

    bool get() const override { return m_level; }
    void set(bool level) override { m_level = level; }
    void pulse() override { m_level = true; m_level = false; } // Simplified pulse

private:
    std::string m_name;
    bool m_level;
};
