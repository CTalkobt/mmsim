#pragma once

#include "isignal_line.h"

/**
 * Simple latching signal line with no side effects on set().
 * Used for vertical-sync signals that are polled by devices.
 */
class SimpleSignalLine : public ISignalLine {
public:
    bool get()  const override { return m_level; }
    void set(bool level) override { m_level = level; }
    void pulse() override { set(false); set(true); }

private:
    bool m_level = true;
};
