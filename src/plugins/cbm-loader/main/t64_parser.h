#pragma once
#include "cbm_disk_image.h"
#include <vector>
#include <cstdint>

class T64Parser : public ICbmDiskImage {
public:
    T64Parser();
    bool open(const std::string& path) override;
    std::vector<CbmDirEntry> getDirectory() const override;
    bool readFile(const std::string& filename, std::vector<uint8_t>& data) override;

private:
    std::vector<uint8_t> m_data;
    struct T64Entry {
        std::string filename;
        uint32_t offset;
        uint32_t size;
        uint8_t type;
    };
    std::vector<T64Entry> m_entries;
};
