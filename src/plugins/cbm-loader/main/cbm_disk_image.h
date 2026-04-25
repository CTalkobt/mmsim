#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct CbmDirEntry {
    std::string filename;
    uint8_t type; // 0x80 | 0x02 = PRG, etc.
    uint16_t sizeBlocks;
};

class ICbmDiskImage {
public:
    virtual ~ICbmDiskImage() {}
    virtual bool open(const std::string& path) = 0;
    virtual std::vector<CbmDirEntry> getDirectory() const = 0;
    virtual bool readFile(const std::string& filename, std::vector<uint8_t>& data) = 0;
    virtual std::string getDiskName() { return ""; }
    virtual std::string getDiskId() { return ""; }
    virtual uint16_t getFreeBlocks() const { return 0; }
};
