#pragma once

#include "execution_observer.h"
#include "breakpoint_list.h"
#include "trace_buffer.h"
#include "stack_trace.h"
#include "libtoolchain/main/symbol_table.h"
#include <vector>
#include <memory>

struct SystemSnapshot {
    std::string label;
    std::vector<uint8_t> cpuState;
    std::vector<uint8_t> busState;
    std::string cartridgePath;
};

class DebugContext : public ExecutionObserver {
public:
    DebugContext(ICore* cpu, IBus* bus);

    bool onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) override;
    void onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) override;
    void onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) override;

    BreakpointList& breakpoints() { return m_breakpoints; }
    TraceBuffer&    trace()       { return m_trace; }
    StackTrace&     stackTrace()  { return m_stackTrace; }
    SymbolTable&    symbols()     { return m_symbols; }

    int  saveSnapshot(const std::string& label);
    bool restoreSnapshot(int index);
    
    // Returns indices of snapshots
    const std::vector<SystemSnapshot>& snapshots() const { return m_snapshots; }

    /**
     * Compare two snapshots and return a list of memory addresses that differ.
     */
    std::vector<uint32_t> diffSnapshots(int idxA, int idxB);

    bool isPaused() const { return m_paused; }
    void resume() { m_resumeSkipAddr = m_lastPausedAddr; m_paused = false; }
    const std::string& lastHitMessage() const { return m_lastHitMessage; }

    ICore* cpu() const { return m_cpu; }
    IBus*  bus() const { return m_bus; }

private:
    void trackStack(ICore* cpu, const DisasmEntry& entry);

    ICore* m_cpu;
    IBus*  m_bus;

    BreakpointList m_breakpoints;
    TraceBuffer    m_trace;
    StackTrace     m_stackTrace;
    SymbolTable    m_symbols;
    std::vector<SystemSnapshot> m_snapshots;
    bool        m_paused          = false;
    uint32_t    m_lastPausedAddr  = ~0u;
    uint32_t    m_resumeSkipAddr  = ~0u;
    std::string m_lastHitMessage;
};
