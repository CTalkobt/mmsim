#pragma once

#include "ibus.h"
#include "util/circular_buffer.h"
#include <vector>
#include <string>

/**
 * Entry in the write log.
 */
struct WriteLogEntry {
    uint32_t address;
    uint8_t  before;
    uint8_t  after;
};

/**
 * ROM Overlay mapping.
 */
struct RomOverlay {
    uint32_t base;
    uint32_t size;
    const uint8_t* data;
    bool writable;
};

/**
 * Generic heap-allocated flat memory bus.
 */
class FlatMemoryBus : public IBus {
public:
    FlatMemoryBus(const std::string& name, uint32_t addrBits);
    virtual ~FlatMemoryBus();

    const BusConfig& config() const override { return m_config; }
    const char*      name()   const override { return m_name.c_str(); }

    uint8_t  read8 (uint32_t addr) override;
    void     write8(uint32_t addr, uint8_t val) override;

    uint8_t  peek8(uint32_t addr) override;

    void reset() override;

    size_t stateSize()             const override;
    void   saveState(uint8_t *buf) const override;
    void   loadState(const uint8_t *buf) override;

    int  writeCount()                                               const override { return (int)m_writeLog.size(); }
    void getWrites(uint32_t *addrs, uint8_t *before,
                            uint8_t *after, int max)                        const override;
    void clearWriteLog() override { m_writeLog.clear(); }

    /**
     * Add a ROM overlay to the bus.
     */
    void addOverlay(uint32_t base, uint32_t size, const uint8_t* data, bool writable = false);

private:
    std::string m_name;
    BusConfig   m_config;
    uint8_t*    m_data;
    size_t      m_size;

    std::vector<RomOverlay> m_overlays;
    CircularBuffer<WriteLogEntry> m_writeLog;

    const RomOverlay* findOverlay(uint32_t addr) const;
};
