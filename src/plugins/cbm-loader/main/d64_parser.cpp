#include "d64_parser.h"
#include <fstream>
#include <algorithm>
#include <cstring>

#include <iostream>

const int D64Parser::s_sectorsPerTrack[] = {
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, // 1-17
    19, 19, 19, 19, 19, 19, 19,                                         // 18-24
    18, 18, 18, 18, 18, 18,                                             // 25-30
    17, 17, 17, 17, 17                                                  // 31-35
};

D64Parser::D64Parser() {}

bool D64Parser::open(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Check if it's a standard 35-track D64 (174848 bytes) 
    // or with error info (196608 bytes)
    if (size != 174848 && size != 196608 && size != 175531) return false;
    
    m_data.resize(size);
    file.read((char*)m_data.data(), size);
    
    return true;
}

uint32_t D64Parser::getSectorOffset(int track, int sector) const {
    if (track < 1 || track > 35) return 0xFFFFFFFF;
    
    uint32_t offset = 0;
    for (int i = 1; i < track; ++i) {
        offset += s_sectorsPerTrack[i - 1] * 256;
    }
    offset += sector * 256;
    return offset;
}

std::vector<CbmDirEntry> D64Parser::getDirectory() const {
    std::vector<CbmDirEntry> entries;
    if (m_data.empty()) return entries;
    
    // First directory sector is at 18, 1
    int track = 18;
    int sector = 1;
    
    while (track != 0) {
        uint32_t offset = getSectorOffset(track, sector);
        if (offset >= m_data.size()) break;
        
        const uint8_t* secData = &m_data[offset];
        
        // Next track/sector
        int nextTrack = secData[0];
        int nextSector = secData[1];
        
        // Each sector has 8 entries of 32 bytes each
        for (int i = 0; i < 8; ++i) {
            const uint8_t* entryData = &secData[i * 32];
            uint8_t type = entryData[2];
            
            if (type != 0) {
                CbmDirEntry entry;
                
                // Filename is at 5-20, padded with 0xA0
                char name[17];
                std::memcpy(name, &entryData[5], 16);
                name[16] = '\0';
                
                // Trim trailing 0xA0
                for (int j = 15; j >= 0; --j) {
                    if ((uint8_t)name[j] == 0xA0) name[j] = '\0';
                    else break;
                }
                
                entry.filename = name;
                entry.type = type;
                entry.sizeBlocks = entryData[30] | (entryData[31] << 8);
                
                entries.push_back(entry);
            }
        }
        
        track = nextTrack;
        sector = nextSector;
        
        // Sanity check to avoid infinite loops
        if (track == 18 && sector == 1) break; 
    }
    
    return entries;
}

bool D64Parser::readFile(const std::string& filename, std::vector<uint8_t>& data) {
    if (m_data.empty()) return false;
std::cerr << "@0" << std::endl;
    auto entries = getDirectory();
    int startTrack = 0;
    int startSector = 0;
std::cerr << "@1" << std::endl;
    // Normalize filename for comparison (case-insensitive? Commodore uses PETSCII)
    // For now simple match
    for (int t = 18, s = 1; t != 0; ) {
        uint32_t offset = getSectorOffset(t, s);
        if (offset >= m_data.size()) break;
        const uint8_t* secData = &m_data[offset];
        
        for (int i = 0; i < 8; ++i) {
            const uint8_t* entryData = &secData[i * 32];
            if (entryData[2] == 0) continue;
            
            char name[17];
            std::memcpy(name, &entryData[5], 16);
            name[16] = '\0';
            for (int j = 15; j >= 0; --j) {
                if ((uint8_t)name[j] == 0xA0) name[j] = '\0';
                else break;
            }
            
            if (filename == name) {
                startTrack = entryData[3];
                startSector = entryData[4];
                goto found;
            }
        }
        t = secData[0];
        s = secData[1];
    }
    
    return false;
    
found:
    int t = startTrack;
    int s = startSector;
    data.clear();
    
    while (t != 0) {
        uint32_t offset = getSectorOffset(t, s);
        if (offset >= m_data.size()) break;
        const uint8_t* secData = &m_data[offset];
        
        int nextT = secData[0];
        int nextS = secData[1];
        
        if (nextT == 0) {
            // Last sector, nextS is number of bytes used
            for (int i = 2; i <= nextS; ++i) {
                data.push_back(secData[i]);
            }
        } else {
            for (int i = 2; i < 256; ++i) {
                data.push_back(secData[i]);
            }
        }
        
        t = nextT;
        s = nextS;
    }

    return true;
}

std::string D64Parser::getDiskName() {
    if (m_data.empty()) return "";

    // Disk name is in directory sector (track 18, sector 1) at offset 0x90
    uint32_t dirOffset = getSectorOffset(18, 1);
    if (dirOffset + 0x90 + 16 >= m_data.size()) return "";

    const uint8_t* dirData = &m_data[dirOffset];
    char name[17];
    std::memcpy(name, &dirData[0x90], 16);
    name[16] = '\0';

    // Trim trailing 0xA0 padding
    for (int i = 15; i >= 0; --i) {
        if ((uint8_t)name[i] == 0xA0) name[i] = '\0';
        else break;
    }

    return name;
}

std::string D64Parser::getDiskId() {
    if (m_data.empty()) return "";

    // Disk ID is in directory sector (track 18, sector 1) at offset 0xA2-0xA3
    uint32_t dirOffset = getSectorOffset(18, 1);
    if (dirOffset + 0xA3 >= m_data.size()) return "";

    const uint8_t* dirData = &m_data[dirOffset];
    char id[3];
    id[0] = (char)dirData[0xA2];
    id[1] = (char)dirData[0xA3];
    id[2] = '\0';

    return id;
}

uint16_t D64Parser::getFreeBlocks() const {
    if (m_data.empty()) return 0;

    // Try to read free blocks from BAM sector (track 18, sector 0)
    // In standard D64, byte 0 of BAM contains free block count for track 18
    // But the total free count might be summed from all tracks or stored elsewhere
    uint32_t bamOffset = getSectorOffset(18, 0);
    if (bamOffset < m_data.size()) {
        // Try reading at byte 0 (free blocks in track 18)
        uint8_t freeInTrack18 = m_data[bamOffset];
        if (freeInTrack18 > 0 && freeInTrack18 < 30) {
            // This looks like a valid track 18 free count
            // For now, sum all track free counts from BAM
            uint16_t totalFree = 0;
            for (int track = 1; track <= 35 && bamOffset + track * 4 < m_data.size(); ++track) {
                totalFree += m_data[bamOffset + (track - 1) * 4];
            }
            if (totalFree > 0) return totalFree;
        }
    }

    // Fallback: Calculate from directory entries
    // 1541 disk capacity: 21*17 + 19*7 + 18*6 + 17*5 = 683 blocks
    uint16_t totalBlocks = 683;
    uint16_t reservedBlocks = 19; // Track 18 is reserved
    uint16_t availableBlocks = totalBlocks - reservedBlocks;

    uint16_t usedBlocks = 0;
    auto entries = getDirectory();
    for (const auto& e : entries) {
        usedBlocks += e.sizeBlocks;
    }

    if (usedBlocks > availableBlocks) return 0;
    return availableBlocks - usedBlocks;
}
