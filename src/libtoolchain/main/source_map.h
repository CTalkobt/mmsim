#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

struct SourceLocation {
    std::string file;
    int line;
};

class SourceMap {
public:
    bool loadKickAssList(const std::string& path);

    SourceLocation addrToSource(uint32_t addr) const;
    uint32_t sourceToAddr(const std::string& file, int line) const;

private:
    std::map<uint32_t, SourceLocation> m_addrToSource;
    std::map<std::string, std::map<int, uint32_t>> m_sourceToAddr;
};
