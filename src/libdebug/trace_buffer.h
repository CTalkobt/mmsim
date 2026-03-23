#pragma once

#include "util/circular_buffer.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

struct TraceEntry {
    uint32_t addr;
    std::string mnemonic;
    std::map<std::string, uint32_t> regs;
    uint64_t cycles;
};

class TraceBuffer {
public:
    TraceBuffer(size_t capacity = 1000);

    void push(const TraceEntry& entry);
    void clear();

    size_t size() const { return m_buffer.size(); }
    const TraceEntry& at(size_t index) const;

private:
    CircularBuffer<TraceEntry> m_buffer;
};
