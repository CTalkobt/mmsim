#include "t64_parser.h"
#include <fstream>
#include <cstring>

T64Parser::T64Parser() {}

bool T64Parser::open(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    m_data.resize(size);
    file.read((char*)m_data.data(), size);
    
    if (size < 64) return false;
    
    // Header "C64 tape image file"
    if (std::memcmp(m_data.data(), "C64 tape image file", 20) != 0) return false;
    
    uint16_t numEntries = m_data[34] | (m_data[35] << 8);
    m_entries.clear();
    
    for (int i = 0; i < numEntries; ++i) {
        uint32_t entryOff = 64 + i * 32;
        if (entryOff + 32 > size) break;
        
        const uint8_t* entryData = &m_data[entryOff];
        if (entryData[0] == 0) continue; // Free entry
        
        T64Entry entry;
        entry.type = entryData[1];
        uint16_t start = entryData[2] | (entryData[3] << 8);
        uint16_t end = entryData[4] | (entryData[5] << 8);
        entry.offset = entryData[8] | (entryData[9] << 8) | 
                       (entryData[10] << 16) | (entryData[11] << 24);
        entry.size = (end > start) ? (end - start) : 0;
        
        char name[17];
        std::memcpy(name, &entryData[12], 16);
        name[16] = '\0';
        for (int j = 15; j >= 0; --j) {
            if (name[j] == ' ' || (uint8_t)name[j] == 0xA0) name[j] = '\0';
            else break;
        }
        entry.filename = name;
        
        m_entries.push_back(entry);
    }
    
    return true;
}

std::vector<CbmDirEntry> T64Parser::getDirectory() {
    std::vector<CbmDirEntry> dir;
    for (const auto& entry : m_entries) {
        CbmDirEntry de;
        de.filename = entry.filename;
        de.type = entry.type;
        de.sizeBlocks = (entry.size + 253) / 254; // Rough estimate
        dir.push_back(de);
    }
    return dir;
}

bool T64Parser::readFile(const std::string& filename, std::vector<uint8_t>& data) {
    for (const auto& entry : m_entries) {
        if (entry.filename == filename) {
            if (entry.offset + entry.size > m_data.size()) return false;
            data.assign(m_data.begin() + entry.offset, m_data.begin() + entry.offset + entry.size);
            return true;
        }
    }
    return false;
}
