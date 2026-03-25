#include "trace_buffer.h"
#include <stdexcept>

TraceBuffer::TraceBuffer(size_t capacity)
    : m_buffer(capacity) {
}

void TraceBuffer::push(const TraceEntry& entry) {
    m_buffer.push(entry);
}

void TraceBuffer::clear() {
    m_buffer.clear();
}

const TraceEntry& TraceBuffer::at(size_t index) const {
    return m_buffer[index];
}
