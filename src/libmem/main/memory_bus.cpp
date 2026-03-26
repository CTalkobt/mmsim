#include "memory_bus.h"
#include "libdebug/main/execution_observer.h"
#include <cstring>
#include <algorithm>

FlatMemoryBus::FlatMemoryBus(const std::string& name, uint32_t addrBits)
    : m_name(name)
{
    m_config.addrBits = addrBits;
    m_config.dataBits = 8;
    m_config.role = BusRole::DATA;
    m_config.littleEndian = true;
    m_config.addrMask = (1u << addrBits) - 1;

    m_size = 1ull << addrBits;
    m_data = new uint8_t[m_size];
    std::memset(m_data, 0, m_size);
}

FlatMemoryBus::~FlatMemoryBus() {
    delete[] m_data;
}

const RomOverlay* FlatMemoryBus::findOverlay(uint32_t addr) const {
    for (const auto& overlay : m_overlays) {
        if (addr >= overlay.base && addr < (overlay.base + overlay.size)) {
            return &overlay;
        }
    }
    return nullptr;
}

uint8_t FlatMemoryBus::read8(uint32_t addr) {
    addr &= m_config.addrMask;
    if (m_ioRead) {
        uint8_t ioVal;
        if (m_ioRead(addr, &ioVal)) {
            if (observer) observer->onMemoryRead(this, addr, ioVal);
            return ioVal;
        }
    }
    uint8_t val;
    const RomOverlay* overlay = findOverlay(addr);
    if (overlay) {
        val = overlay->data[addr - overlay->base];
    } else {
        val = m_data[addr];
    }
    if (observer) observer->onMemoryRead(this, addr, val);
    return val;
}

void FlatMemoryBus::write8(uint32_t addr, uint8_t val) {
    addr &= m_config.addrMask;
    if (m_ioWrite && m_ioWrite(addr, val)) {
        if (observer) observer->onMemoryWrite(this, addr, m_data[addr], val);
        return;
    }
    const RomOverlay* overlay = findOverlay(addr);

    uint8_t before = peek8(addr);
    
    if (overlay) {
        if (overlay->writable) {
            m_data[addr] = val;
        } else {
            // ROM: write ignored
            if (observer) observer->onMemoryWrite(this, addr, before, val);
            return;
        }
    } else {
        m_data[addr] = val;
    }

    if (observer) observer->onMemoryWrite(this, addr, before, val);
    
    m_writeLog.push({addr, before, val});
}

uint8_t FlatMemoryBus::peek8(uint32_t addr) {
    addr &= m_config.addrMask;
    const RomOverlay* overlay = findOverlay(addr);
    if (overlay) {
        return overlay->data[addr - overlay->base];
    }
    return m_data[addr];
}

void FlatMemoryBus::reset() {
    // Roadmap doesn't say reset clears memory, usually it doesn't for RAM.
    // But it might clear the write log.
    clearWriteLog();
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
    int count = std::min((int)m_writeLog.size(), max);
    for (int i = 0; i < count; ++i) {
        addrs[i] = m_writeLog[i].address;
        before[i] = m_writeLog[i].before;
        after[i] = m_writeLog[i].after;
    }
}

void FlatMemoryBus::addOverlay(uint32_t base, uint32_t size, const uint8_t* data, bool writable) {
    m_overlays.push_back({base, size, data, writable});
}

void FlatMemoryBus::setIoHooks(std::function<bool(uint32_t, uint8_t*)> readFn,
                                std::function<bool(uint32_t, uint8_t)>  writeFn) {
    m_ioRead  = std::move(readFn);
    m_ioWrite = std::move(writeFn);
}
