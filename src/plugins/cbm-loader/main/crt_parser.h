#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct CrtHeader {
    char signature[16];
    uint32_t headerLen;
    uint16_t version;
    uint16_t cartType;
    uint8_t exrom;
    uint8_t game;
    char reserved[6];
    char cartName[32];
};

struct CrtChipPacket {
    char signature[4];
    uint32_t packetLen;
    uint16_t chipType;
    uint16_t bankNum;
    uint16_t loadAddr;
    uint16_t imageSize;
    std::vector<uint8_t> data;
};

class CrtParser {
public:
    bool parse(const std::string& path);

    const CrtHeader& header() const { return m_header; }
    const std::vector<CrtChipPacket>& chips() const { return m_chips; }

private:
    CrtHeader m_header;
    std::vector<CrtChipPacket> m_chips;

    uint16_t read16BE(std::ifstream& f);
    uint32_t read32BE(std::ifstream& f);
};
