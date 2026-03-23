#include "assembler_6502.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cstdint>

Assembler6502::Assembler6502() {
    initTable();
}

void Assembler6502::add(const std::string& mnem, const std::string& mode, uint8_t op, int sz) {
    m_mnemonics[mnem][mode] = {op, sz};
}

void Assembler6502::initTable() {
    // A subset of official opcodes for the mini-assembler
    add("LDA", "imm", 0xA9, 2); add("LDA", "zp", 0xA5, 2); add("LDA", "abs", 0xAD, 3);
    add("LDX", "imm", 0xA2, 2); add("LDX", "zp", 0xA6, 2); add("LDX", "abs", 0xAE, 3);
    add("LDY", "imm", 0xA0, 2); add("LDY", "zp", 0xA4, 2); add("LDY", "abs", 0xAC, 3);
    add("STA", "zp", 0x85, 2); add("STA", "abs", 0x8D, 3);
    add("STX", "zp", 0x86, 2); add("STX", "abs", 0x8E, 3);
    add("STY", "zp", 0x84, 2); add("STY", "abs", 0x8C, 3);
    add("NOP", "imp", 0xEA, 1);
    add("TAX", "imp", 0xAA, 1); add("TAY", "imp", 0xA8, 1);
    add("TXA", "imp", 0x8A, 1); add("TYA", "imp", 0x98, 1);
    add("INX", "imp", 0xE8, 1); add("INY", "imp", 0xC8, 1);
    add("DEX", "imp", 0xCA, 1); add("DEY", "imp", 0x88, 1);
    add("CLC", "imp", 0x18, 1); add("SEC", "imp", 0x38, 1);
    add("ADC", "imm", 0x69, 2); add("SBC", "imm", 0xE9, 2);
    add("JMP", "abs", 0x4C, 3); add("JSR", "abs", 0x20, 3);
    add("RTS", "imp", 0x60, 1); add("RTI", "imp", 0x40, 1);
}

AssemblerResult Assembler6502::assemble(const std::string& sourcePath, const std::string& outputPath) {
    (void)sourcePath; (void)outputPath;
    return {false, "File assembly not supported by native mini-assembler", "", "", "", 0, 0};
}

int Assembler6502::assembleLine(const std::string& line, uint8_t* buf, int bufsz) {
    if (line.empty()) return 0;

    std::stringstream ss(line);
    std::string mnem, oper;
    ss >> mnem;
    std::transform(mnem.begin(), mnem.end(), mnem.begin(), ::toupper);

    if (m_mnemonics.count(mnem) == 0) return -1;

    // Remaining part is operands
    std::string remaining;
    std::getline(ss, remaining);
    // trim whitespace
    if (!remaining.empty()) {
        remaining.erase(0, remaining.find_first_not_of(" \t"));
        size_t last = remaining.find_last_not_of(" \t");
        if (std::string::npos != last) {
            remaining.erase(last + 1);
        }
    }

    std::string mode = "imp";
    uint32_t val = 0;

    if (remaining.empty()) {
        mode = "imp";
    } else if (remaining[0] == '#') {
        mode = "imm";
        // parse hex or dec
        if (remaining.size() > 2 && remaining[1] == '$') {
            val = std::stoul(remaining.substr(2), nullptr, 16);
        } else {
            val = std::stoul(remaining.substr(1));
        }
    } else if (remaining[0] == '$') {
        val = std::stoul(remaining.substr(1), nullptr, 16);
        mode = (val <= 0xFF) ? "zp" : "abs";
    } else if (std::isdigit(remaining[0])) {
        val = std::stoul(remaining);
        mode = (val <= 0xFF) ? "zp" : "abs";
    } else {
        return -1; // Unsupported mode for now
    }

    if (m_mnemonics[mnem].count(mode) == 0) {
        // Fallback: if "zp" fails, try "abs"
        if (mode == "zp" && m_mnemonics[mnem].count("abs")) {
            mode = "abs";
        } else {
            return -1;
        }
    }

    const auto& info = m_mnemonics[mnem][mode];
    if (info.size > bufsz) return -1;

    buf[0] = info.opcode;
    if (info.size > 1) {
        buf[1] = val & 0xFF;
    }
    if (info.size > 2) {
        buf[2] = (val >> 8) & 0xFF;
    }

    return info.size;
}
