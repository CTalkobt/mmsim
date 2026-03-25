#include "symbol_table.h"
#include <fstream>
#include <sstream>
#include <iostream>

void SymbolTable::addSymbol(uint32_t addr, const std::string& label) {
    m_addrToLabel[addr] = label;
    m_labelToAddr[label] = addr;
}

std::string SymbolTable::getLabel(uint32_t addr) const {
    auto it = m_addrToLabel.find(addr);
    if (it != m_addrToLabel.end()) return it->second;
    return "";
}

bool SymbolTable::hasSymbol(uint32_t addr) const {
    return m_addrToLabel.count(addr) > 0;
}

std::string SymbolTable::nearest(uint32_t addr, uint32_t& offset) const {
    if (m_addrToLabel.empty()) return "";

    auto it = m_addrToLabel.upper_bound(addr);
    if (it == m_addrToLabel.begin()) {
        // addr is before the first symbol
        return "";
    }

    // upper_bound returns the first element > addr, so we need the previous one
    --it;
    offset = addr - it->first;
    return it->second;
}

bool SymbolTable::loadKickAssSym(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        // KickAss sym format: .label name=$addr [some extra stuff]
        if (line.size() < 10 || line[0] != '.') continue;

        size_t labelStart = 7; // after ".label "
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string label = line.substr(labelStart, eqPos - labelStart);
        
        size_t dolPos = line.find('$', eqPos);
        if (dolPos == std::string::npos) continue;

        std::string addrStr = line.substr(dolPos + 1);
        uint32_t addr;
        std::stringstream ss;
        ss << std::hex << addrStr;
        ss >> addr;

        addSymbol(addr, label);
    }

    return true;
}
