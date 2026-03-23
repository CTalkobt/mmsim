#include "debug_context.h"
#include <iostream>

DebugContext::DebugContext(ICore* cpu, IBus* bus)
    : m_cpu(cpu), m_bus(bus) {
}

void DebugContext::onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) {
    TraceEntry te;
    te.addr = entry.addr;
    te.mnemonic = entry.complete;
    te.cycles = cpu->cycles();
    
    for (int i = 0; i < cpu->regCount(); ++i) {
        const auto* desc = cpu->regDescriptor(i);
        if (!(desc->flags & REGFLAG_INTERNAL)) {
            te.regs[desc->name] = cpu->regRead(i);
        }
    }
    
    m_trace.push(te);

    // Call non-const checkExec
    m_breakpoints.checkExec(entry.addr);
}

void DebugContext::onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) {
    (void)bus; (void)before; (void)after;
    m_breakpoints.checkWrite(addr);
}

void DebugContext::onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) {
    (void)bus; (void)val;
    m_breakpoints.checkRead(addr);
}

int DebugContext::saveSnapshot(const std::string& label) {
    SystemSnapshot snap;
    snap.label = label;
    
    snap.cpuState.resize(m_cpu->stateSize());
    m_cpu->saveState(snap.cpuState.data());
    
    snap.busState.resize(m_bus->stateSize());
    m_bus->saveState(snap.busState.data());
    
    m_snapshots.push_back(std::move(snap));
    return (int)m_snapshots.size() - 1;
}

bool DebugContext::restoreSnapshot(int index) {
    if (index < 0 || index >= (int)m_snapshots.size()) return false;
    
    const auto& snap = m_snapshots[index];
    m_cpu->loadState(snap.cpuState.data());
    m_bus->loadState(snap.busState.data());
    
    return true;
}

std::vector<uint32_t> DebugContext::diffSnapshots(int idxA, int idxB) {
    std::vector<uint32_t> diffs;
    if (idxA < 0 || idxA >= (int)m_snapshots.size()) return diffs;
    if (idxB < 0 || idxB >= (int)m_snapshots.size()) return diffs;
    
    const auto& snapA = m_snapshots[idxA];
    const auto& snapB = m_snapshots[idxB];
    
    if (snapA.busState.size() != snapB.busState.size()) return diffs;
    
    for (uint32_t i = 0; i < snapA.busState.size(); ++i) {
        if (snapA.busState[i] != snapB.busState[i]) {
            diffs.push_back(i);
        }
    }
    
    return diffs;
}
