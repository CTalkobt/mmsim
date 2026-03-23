#include "source_map.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

bool SourceMap::loadKickAssList(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    // Basic regex to match something like "[1000] 4c 00 10  1  main: jmp main"
    // This is very simplified.
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] != '[') continue;

        size_t endBracket = line.find(']');
        if (endBracket == std::string::npos) continue;

        std::string addrStr = line.substr(1, endBracket - 1);
        uint32_t addr;
        std::stringstream ss;
        ss << std::hex << addrStr;
        ss >> addr;

        // Find the line number. In KickAss list files, it's often after the bytes.
        // For now, let's just store the address.
        // Full parser would be more complex.
        
        m_addrToSource[addr] = {path, 0}; // Line 0 as placeholder
    }

    return true;
}

SourceLocation SourceMap::addrToSource(uint32_t addr) const {
    auto it = m_addrToSource.find(addr);
    if (it != m_addrToSource.end()) return it->second;
    return {"", -1};
}

uint32_t SourceMap::sourceToAddr(const std::string& file, int line) const {
    auto itFile = m_sourceToAddr.find(file);
    if (itFile != m_sourceToAddr.end()) {
        auto itLine = itFile->second.find(line);
        if (itLine != itFile->second.end()) return itLine->second;
    }
    return 0xFFFFFFFF;
}
