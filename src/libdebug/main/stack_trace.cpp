#include "stack_trace.h"

const char* stackPushTypeName(StackPushType t) {
    switch (t) {
        case StackPushType::CALL: return "CALL";
        case StackPushType::PHA:  return "PHA";
        case StackPushType::PHX:  return "PHX";
        case StackPushType::PHY:  return "PHY";
        case StackPushType::PHP:  return "PHP";
        case StackPushType::PHZ:  return "PHZ";
        case StackPushType::BRK:  return "BRK";
    }
    return "?";
}

void StackTrace::push(StackPushType type, uint32_t pc, uint32_t value) {
    m_entries.push_back({type, pc, value});
}

void StackTrace::pop() {
    if (!m_entries.empty())
        m_entries.pop_back();
}

void StackTrace::clear() {
    m_entries.clear();
}

std::vector<StackEntry> StackTrace::recent(int n) const {
    std::vector<StackEntry> result;
    int total = (int)m_entries.size();
    int start = (n > 0 && total > n) ? total - n : 0;
    result.reserve(total - start);
    for (int i = total - 1; i >= start; --i)
        result.push_back(m_entries[i]);
    return result;
}
