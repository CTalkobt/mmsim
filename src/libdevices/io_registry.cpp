#include "io_registry.h"
#include <algorithm>

void IORegistry::registerHandler(IOHandler* handler) {
    m_handlers.push_back(handler);
    // Sort by base address to establish a predictable priority
    std::sort(m_handlers.begin(), m_handlers.end(), [](IOHandler* a, IOHandler* b) {
        return a->baseAddr() < b->baseAddr();
    });
}

bool IORegistry::dispatchRead(IBus* bus, uint32_t addr, uint8_t* val) {
    for (auto* handler : m_handlers) {
        if (handler->ioRead(bus, addr, val)) {
            return true;
        }
    }
    return false;
}

bool IORegistry::dispatchWrite(IBus* bus, uint32_t addr, uint8_t val) {
    for (auto* handler : m_handlers) {
        if (handler->ioWrite(bus, addr, val)) {
            return true;
        }
    }
    return false;
}

void IORegistry::tickAll(uint64_t cycles) {
    for (auto* handler : m_handlers) {
        handler->tick(cycles);
    }
}

void IORegistry::resetAll() {
    for (auto* handler : m_handlers) {
        handler->reset();
    }
}

void IORegistry::enumerate(std::vector<IOHandler*>& list) {
    for (auto* handler : m_handlers) {
        list.push_back(handler);
    }
}

void IORegistry::clear() {
    m_handlers.clear();
}
