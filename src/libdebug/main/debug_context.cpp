#include "debug_context.h"
#include "include/mmemu_plugin_api.h"
#include "libcore/main/image_loader.h"
#include <iostream>
#include <sstream>
#include <string>

static std::string toHex(uint32_t v) {
    std::ostringstream ss;
    ss << std::uppercase << std::hex << v;
    return ss.str();
}

DebugContext::DebugContext(ICore* cpu, IBus* bus)
    : m_cpu(cpu), m_bus(bus) {
}

bool DebugContext::onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) {
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

    // On resume, skip the breakpoint at the address we just paused on (one step).
    if (entry.addr == m_resumeSkipAddr) {
        m_resumeSkipAddr = ~0u;
        return true;
    }

    if (auto* bp = m_breakpoints.checkExec(entry.addr, cpu, bus)) {
        std::string msg = "Execution breakpoint " + std::to_string(bp->id) + " hit at $" + toHex(entry.addr);
        m_cpu->log(SIM_LOG_INFO, msg.c_str());
        m_lastPausedAddr = entry.addr;
        m_paused = true;
        return false;
    }
    return true;
}

void DebugContext::onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) {
    (void)bus; (void)before; (void)after;
    if (auto* bp = m_breakpoints.checkWrite(addr, m_cpu, bus)) {
        std::string msg = "Write watchpoint " + std::to_string(bp->id) + " hit at $" + toHex(addr);
        m_cpu->log(SIM_LOG_INFO, msg.c_str());
        m_paused = true;
    }
}

void DebugContext::onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) {
    (void)bus; (void)val;
    if (auto* bp = m_breakpoints.checkRead(addr, m_cpu, bus)) {
        std::string msg = "Read watchpoint " + std::to_string(bp->id) + " hit at $" + toHex(addr);
        m_cpu->log(SIM_LOG_INFO, msg.c_str());
        m_paused = true;
    }
}

int DebugContext::saveSnapshot(const std::string& label) {
    SystemSnapshot snap;
    snap.label = label;
    
    snap.cpuState.resize(m_cpu->stateSize());
    m_cpu->saveState(snap.cpuState.data());
    
    snap.busState.resize(m_bus->stateSize());
    m_bus->saveState(snap.busState.data());

    auto* cart = ImageLoaderRegistry::instance().getActiveCartridge(m_bus);
    if (cart) {
        snap.cartridgePath = cart->metadata().imagePath;
    }
    
    m_snapshots.push_back(std::move(snap));
    return (int)m_snapshots.size() - 1;
}

bool DebugContext::restoreSnapshot(int index) {
    if (index < 0 || index >= (int)m_snapshots.size()) return false;
    
    const auto& snap = m_snapshots[index];
    m_cpu->loadState(snap.cpuState.data());
    m_bus->loadState(snap.busState.data());

    // Restore cartridge if path is set and different from current
    auto* currentCart = ImageLoaderRegistry::instance().getActiveCartridge(m_bus);
    std::string currentPath = currentCart ? currentCart->metadata().imagePath : "";
    
    if (snap.cartridgePath != currentPath) {
        if (currentCart) {
            currentCart->eject(m_bus);
            ImageLoaderRegistry::instance().setActiveCartridge(m_bus, nullptr);
        }
        if (!snap.cartridgePath.empty()) {
            auto newCart = ImageLoaderRegistry::instance().createCartridgeHandler(snap.cartridgePath);
            if (newCart) {
                if (newCart->attach(m_bus, nullptr)) {
                    ImageLoaderRegistry::instance().setActiveCartridge(m_bus, std::move(newCart));
                }
            }
        }
    }
    
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
