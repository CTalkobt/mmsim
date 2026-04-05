#include "memory_bus.h"
#include "libdebug/main/execution_observer.h"
#include <cstring>
#include <algorithm>
#include <iostream>

FlatMemoryBus::FlatMemoryBus(const std::string& name, uint32_t addrBits)
    : m_name(name)
{
    m_config.addrBits = addrBits;
    m_config.dataBits = 8;
    m_config.addrMask = (1ULL << addrBits) - 1;
    m_config.littleEndian = true;
    m_config.role = BusRole::DATA;

    m_size = 1ULL << addrBits;
    m_data = new uint8_t[m_size];
    std::memset(m_data, 0xFF, m_size);
}

FlatMemoryBus::~FlatMemoryBus() {
    delete[] m_data;
}

uint8_t FlatMemoryBus::read8(uint32_t addr) {
    addr &= m_config.addrMask;
    uint8_t val;
    bool handled = false;
    if (m_ioRead && isIoAddress(addr)) {
        if (m_ioRead(this, addr, &val)) {
            handled = true;
        }
    }
    
    if (!handled) {
        val = read8Raw(addr);
    }

    if (m_observer) m_observer->onMemoryRead(this, addr, val);
    
    // Debug: Trace XL OS entry if it looks like we are in a crash loop
    // if (addr >= 0xC000 && addr < 0xC100) std::cerr << "R " << std::hex << addr << "=" << (int)val << std::endl;

    return val;
}

uint8_t FlatMemoryBus::read8Raw(uint32_t addr) {
    addr &= m_config.addrMask;
    const RomOverlay* overlay = findOverlay(addr);
    if (overlay) {
        return overlay->data[addr - overlay->base];
    }
    return m_data[addr];
}

void FlatMemoryBus::write8(uint32_t addr, uint8_t val) {
    addr &= m_config.addrMask;
    uint8_t before = peek8(addr);
    bool handled = false;

    if (m_ioWrite && isIoAddress(addr)) {
        if (m_ioWrite(this, addr, val)) {
            handled = true;
        }
    }

    if (!handled) {
        const RomOverlay* overlay = findOverlay(addr);
        if (overlay) {
            if (overlay->writable) {
                const_cast<uint8_t*>(overlay->data)[addr - overlay->base] = val;
            }
            handled = true;
        }
    }

    if (!handled) {
        m_data[addr] = val;
        m_writeLog.push({addr, before, val});
    }

    if (m_observer) m_observer->onMemoryWrite(this, addr, before, val);
}

uint8_t FlatMemoryBus::peek8(uint32_t addr) {
    addr &= m_config.addrMask;
    if (m_ioRead && isIoAddress(addr)) {
        uint8_t ioVal;
        if (m_ioRead(this, addr, &ioVal)) {
            return ioVal;
        }
    }
    return peek8Raw(addr);
}

uint8_t FlatMemoryBus::peek8Raw(uint32_t addr) {
    addr &= m_config.addrMask;
    const RomOverlay* overlay = findOverlay(addr);
    if (overlay) {
        return overlay->data[addr - overlay->base];
    }
    return m_data[addr];
}

void FlatMemoryBus::reset() {
    std::memset(m_data, 0xFF, m_size);
}

size_t FlatMemoryBus::stateSize() const {
    return m_size;
}

void FlatMemoryBus::saveState(uint8_t *buf) const {
    std::memcpy(buf, m_data, m_size);
}

void FlatMemoryBus::loadState(const uint8_t *buf) {
    std::memcpy(m_data, buf, m_size);
}

void FlatMemoryBus::getWrites(uint32_t *addrs, uint8_t *before,
                                uint8_t *after, int max) const {
    int count = std::min(max, (int)m_writeLog.size());
    for (int i = 0; i < count; ++i) {
        const auto& entry = m_writeLog[m_writeLog.size() - count + i];
        addrs[i] = entry.address;
        before[i] = entry.before;
        after[i] = entry.after;
    }
}

void FlatMemoryBus::addOverlay(uint32_t base, uint32_t size, const uint8_t* data, bool writable) {
    m_overlays.push_back({base, size, data, writable});
}

void FlatMemoryBus::addRomOverlay(uint32_t base, uint32_t size, const uint8_t* data) {
    addOverlay(base, size, data, false);
}

void FlatMemoryBus::removeRomOverlay(uint32_t base) {
    auto it = std::remove_if(m_overlays.begin(), m_overlays.end(), [base](const RomOverlay& o) {
        return o.base == base;
    });
    m_overlays.erase(it, m_overlays.end());
}

void FlatMemoryBus::setIoHooks(std::function<bool(IBus*, uint32_t, uint8_t*)> readFn,
                                std::function<bool(IBus*, uint32_t, uint8_t)>  writeFn) {
    m_ioRead  = std::move(readFn);
    m_ioWrite = std::move(writeFn);
}

void FlatMemoryBus::addIoRange(uint32_t base, uint32_t size) {
    m_ioRanges.push_back({base, size});
}

bool FlatMemoryBus::isIoAddress(uint32_t addr) const {
    if (m_ioRanges.empty()) return true;
    for (const auto& range : m_ioRanges) {
        if (addr >= range.first && addr < (range.first + range.second)) {
            return true;
        }
    }
    return false;
}

const RomOverlay* FlatMemoryBus::findOverlay(uint32_t addr) const {
    for (auto it = m_overlays.rbegin(); it != m_overlays.rend(); ++it) {
        if (addr >= it->base && addr < (it->base + it->size)) {
            return &(*it);
        }
    }
    return nullptr;
}

