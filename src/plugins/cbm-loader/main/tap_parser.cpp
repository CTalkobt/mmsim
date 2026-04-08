#include "tap_parser.h"
#include <fstream>
#include <cstring>

bool TapArchive::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    f.read(m_header.signature, 12);
    if (std::strncmp(m_header.signature, "C64-TAPE-RAW", 12) != 0) return false;

    f.read((char*)&m_header.version, 1);
    f.read((char*)m_header.reserved, 3);
    
    uint8_t sizeBytes[4];
    f.read((char*)sizeBytes, 4);
    m_header.dataSize = sizeBytes[0] | (sizeBytes[1] << 8) | (sizeBytes[2] << 16) | (sizeBytes[3] << 24);

    m_data.resize(m_header.dataSize);
    f.read((char*)m_data.data(), m_header.dataSize);

    return true;
}

uint32_t TapArchive::nextPulse(uint32_t& offset) const {
    if (offset >= m_data.size()) return 0;

    uint8_t val = m_data[offset++];
    if (val > 0) {
        return val * 8;
    } else {
        // Overflow or special case. In version 0/1, it is followed by 3 bytes for 24-bit value.
        if (offset + 3 > m_data.size()) return 0;
        uint32_t longVal = m_data[offset] | (m_data[offset+1] << 8) | (m_data[offset+2] << 16);
        offset += 3;
        return longVal;
    }
}
