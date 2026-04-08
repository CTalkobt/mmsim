#include "symbol_table.h"
#include <fstream>
#include <sstream>
#include <iostream>

void SymbolTable::addSymbol(uint32_t addr, const std::string& label) {
    m_addrToLabel[addr] = label;
    m_labelToAddr[label] = addr;
}

void SymbolTable::removeSymbol(const std::string& label) {
    auto it = m_labelToAddr.find(label);
    if (it != m_labelToAddr.end()) {
        m_addrToLabel.erase(it->second);
        m_labelToAddr.erase(it);
    }
}

void SymbolTable::clear() {
    m_addrToLabel.clear();
    m_labelToAddr.clear();
}

std::string SymbolTable::getLabel(uint32_t addr) const {
    auto it = m_addrToLabel.find(addr);
    if (it != m_addrToLabel.end()) return it->second;
    return "";
}

uint32_t SymbolTable::getAddress(const std::string& label) const {
    auto it = m_labelToAddr.find(label);
    if (it != m_labelToAddr.end()) return it->second;
    return 0xFFFFFFFF;
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

bool SymbolTable::loadSym(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        // Format: "label = value" or "label value"
        if (line.empty()) continue;

        // Strip leading/trailing whitespace
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        size_t last = line.find_last_not_of(" \t");
        std::string trimmed = line.substr(first, last - first + 1);

        // Find separator (either '=' or whitespace)
        size_t sep = trimmed.find('=');
        std::string label, valStr;
        if (sep != std::string::npos) {
            label = trimmed.substr(0, sep);
            valStr = trimmed.substr(sep + 1);
        } else {
            sep = trimmed.find_first_of(" \t");
            if (sep == std::string::npos) continue;
            label = trimmed.substr(0, sep);
            valStr = trimmed.substr(sep + 1);
        }

        // Clean label and value
        auto trim = [](std::string& s) {
            size_t f = s.find_first_not_of(" \t");
            if (f == std::string::npos) { s = ""; return; }
            size_t l = s.find_last_not_of(" \t");
            s = s.substr(f, l - f + 1);
        };
        trim(label);
        trim(valStr);

        if (label.empty() || valStr.empty()) continue;

        uint32_t addr;
        if (valStr[0] == '$') {
            std::stringstream ss;
            ss << std::hex << valStr.substr(1);
            ss >> addr;
        } else if (valStr[0] == '%') {
            try {
                addr = std::stoul(valStr.substr(1), nullptr, 2);
            } catch (...) { continue; }
        } else {
            try {
                addr = std::stoul(valStr, nullptr, 0);
            } catch (...) { continue; }
        }

        addSymbol(addr, label);
    }

    return true;
}
