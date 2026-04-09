#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct TapHeader {
    char signature[12];
    uint8_t version;
    uint8_t reserved[3];
    uint32_t dataSize;
};

class TapArchive {
public:
    bool load(const std::string& path);

    uint8_t version() const { return m_header.version; }
    uint32_t dataSize() const { return m_header.dataSize; }

    /**
     * Get the next pulse length in CPU cycles.
     * @param offset Current offset in data (updated).
     * @return Pulse length in cycles, or 0 if end of data.
     */
    uint32_t nextPulse(uint32_t& offset) const;

    const std::vector<uint8_t>& data() const { return m_data; }

    // Recording API
    void startRecording();
    void appendPulse(uint32_t cycles);
    bool saveRecording(const std::string& path) const;
    bool isRecording() const { return m_recording; }
    void stopRecording() { m_recording = false; }
    uint32_t recordedPulseCount() const { return (uint32_t)m_recordBuf.size(); }

private:
    TapHeader m_header;
    std::vector<uint8_t> m_data;

    bool                 m_recording = false;
    std::vector<uint8_t> m_recordBuf;
};
