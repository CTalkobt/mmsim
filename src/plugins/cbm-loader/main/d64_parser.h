#pragma once
#include "cbm_disk_image.h"
#include <vector>
#include <cstdint>

class D64Parser : public ICbmDiskImage {
public:
    D64Parser();
    bool open(const std::string& path) override;
    std::vector<CbmDirEntry> getDirectory() override;
    bool readFile(const std::string& filename, std::vector<uint8_t>& data) override;
    std::string getDiskName() override;
    std::string getDiskId() override;
    uint16_t getFreeBlocks() const;

private:
    std::vector<uint8_t> m_data;
    
    // 1541 D64 sector counts per track
    static const int s_sectorsPerTrack[];
    
    uint32_t getSectorOffset(int track, int sector) const;
};
