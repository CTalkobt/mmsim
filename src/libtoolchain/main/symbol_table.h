#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <vector>

/**
 * Maps memory addresses to labels and vice versa.
 */
class SymbolTable {
public:
    void addSymbol(uint32_t addr, const std::string& label);
    void removeSymbol(const std::string& label);
    void clear();
    std::string getLabel(uint32_t addr) const;
    bool hasSymbol(uint32_t addr) const;

    /**
     * Find the nearest label at or before the given address.
     * @param addr The address to look up.
     * @param offset Out parameter: distance from the label to addr.
     * @return Label name, or empty string if none found.
     */
    std::string nearest(uint32_t addr, uint32_t& offset) const;

    /**
     * Load symbols from a KickAssembler .sym file.
     */
    bool loadKickAssSym(const std::string& path);

    /**
     * Load symbols from a .sym file.
     * Format: "label = value" or "label <whitespace> value"
     * Value can be hex ($prefix) or decimal.
     */
    bool loadSym(const std::string& path);

    const std::map<uint32_t, std::string>& symbols() const { return m_addrToLabel; }
    uint32_t getAddress(const std::string& label) const;

private:
    std::map<uint32_t, std::string> m_addrToLabel;
    std::map<std::string, uint32_t> m_labelToAddr;
};
