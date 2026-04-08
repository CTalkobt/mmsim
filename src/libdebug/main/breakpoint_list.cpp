#include "breakpoint_list.h"
#include "debug_context.h"
#include "expression_evaluator.h"
#include <algorithm>

int BreakpointList::add(uint32_t addr, BreakpointType type) {
    int id = m_nextId++;
    m_breakpoints.push_back({addr, type, "", 0, true, id});
    return id;
}

void BreakpointList::remove(int id) {
    m_breakpoints.erase(
        std::remove_if(m_breakpoints.begin(), m_breakpoints.end(),
                       [id](const Breakpoint& b) { return b.id == id; }),
        m_breakpoints.end());
}

void BreakpointList::setEnabled(int id, bool enabled) {
    for (auto& b : m_breakpoints) {
        if (b.id == id) {
            b.enabled = enabled;
            return;
        }
    }
}

Breakpoint* BreakpointList::checkExec(uint32_t addr, DebugContext* dbg) {
    for (auto& b : m_breakpoints) {
        if (b.enabled && b.type == BreakpointType::EXEC && b.addr == addr) {
            if (ExpressionEvaluator::evaluateCondition(b.condition, dbg)) {
                b.hitCount++;
                return &b;
            }
        }
    }
    return nullptr;
}

Breakpoint* BreakpointList::checkWrite(uint32_t addr, DebugContext* dbg) {
    for (auto& b : m_breakpoints) {
        if (b.enabled && b.type == BreakpointType::WRITE_WATCH && b.addr == addr) {
            if (ExpressionEvaluator::evaluateCondition(b.condition, dbg)) {
                b.hitCount++;
                return &b;
            }
        }
    }
    return nullptr;
}

Breakpoint* BreakpointList::checkRead(uint32_t addr, DebugContext* dbg) {
    for (auto& b : m_breakpoints) {
        if (b.enabled && b.type == BreakpointType::READ_WATCH && b.addr == addr) {
            if (ExpressionEvaluator::evaluateCondition(b.condition, dbg)) {
                b.hitCount++;
                return &b;
            }
        }
    }
    return nullptr;
}
