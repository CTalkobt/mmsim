#pragma once
#include "cbm_disk_image.h"
#include <vector>
#include <cstdint>

class G64Parser : public ICbmDiskImage {
public:
    G64Parser();
    bool open(const std::string& path) override;
    std::vector<CbmDirEntry> getDirectory() override;
    bool readFile(const std::string& filename, std::vector<uint8_t>& data) override;

    // G64 specific: get raw GCR data for a track
    bool getTrackData(int track, std::vector<uint8_t>& gcrData);

private:
    std::vector<uint8_t> m_data;
    struct TrackInfo {
        uint32_t offset;
        uint32_t speed;
    };
    std::vector<TrackInfo> m_tracks;
};
