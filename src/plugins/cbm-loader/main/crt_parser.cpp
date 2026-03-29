#include "crt_parser.h"
#include <fstream>
#include <iostream>
#include <cstring>

uint16_t CrtParser::read16BE(std::ifstream& f) {
    uint8_t b[2];
    f.read((char*)b, 2);
    return (b[0] << 8) | b[1];
}

uint32_t CrtParser::read32BE(std::ifstream& f) {
    uint8_t b[4];
    f.read((char*)b, 4);
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

bool CrtParser::parse(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;

    f.read(m_header.signature, 16);
    if (std::strncmp(m_header.signature, "C64 CARTRIDGE", 13) != 0) return false;

    m_header.headerLen = read32BE(f);
    m_header.version = read16BE(f);
    m_header.cartType = read16BE(f);
    f.read((char*)&m_header.exrom, 1);
    f.read((char*)&m_header.game, 1);
    f.read(m_header.reserved, 6);
    f.read(m_header.cartName, 32);

    // Seek to the end of header if headerLen > 0x40
    if (m_header.headerLen > 0x40) {
        f.seekg(m_header.headerLen, std::ios::beg);
    } else {
        f.seekg(0x40, std::ios::beg);
    }

    while (f.peek() != EOF) {
        CrtChipPacket chip;
        f.read(chip.signature, 4);
        if (std::strncmp(chip.signature, "CHIP", 4) != 0) break;

        chip.packetLen = read32BE(f);
        chip.chipType = read16BE(f);
        chip.bankNum = read16BE(f);
        chip.loadAddr = read16BE(f);
        chip.imageSize = read16BE(f);

        chip.data.resize(chip.imageSize);
        f.read((char*)chip.data.data(), chip.imageSize);

        m_chips.push_back(std::move(chip));
    }

    return true;
}
