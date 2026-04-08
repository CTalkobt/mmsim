#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class BreakpointType {
    EXEC,
    READ_WATCH,
    WRITE_WATCH
};

struct Breakpoint {
    uint32_t       addr;
    BreakpointType type;
    std::string    condition;
    int            hitCount;
    bool           enabled;
    int            id;
};

class DebugContext;

class BreakpointList {
public:
    int  add(uint32_t addr, BreakpointType type);
    void remove(int id);
    void setEnabled(int id, bool enabled);

    Breakpoint* checkExec(uint32_t addr, DebugContext* dbg);
    Breakpoint* checkWrite(uint32_t addr, DebugContext* dbg);
    Breakpoint* checkRead(uint32_t addr, DebugContext* dbg);

    const std::vector<Breakpoint>& breakpoints() const { return m_breakpoints; }

private:
    std::vector<Breakpoint> m_breakpoints;
    int m_nextId = 1;
};
