#pragma once

#include "libtoolchain/iassembler.h"
#include <map>
#include <string>
#include <vector>
#include <cstdint>

class Assembler6502 : public IAssembler {
public:
    Assembler6502();

    const char* name() const override { return "Native6502"; }
    bool isaSupported(const std::string& isa) const override { return isa == "6502"; }

    AssemblerResult assemble(const std::string& sourcePath, const std::string& outputPath) override;
    int assembleLine(const std::string& line, uint8_t* buf, int bufsz) override;

private:
    struct OpcodeInfo {
        uint8_t opcode;
        int size;
    };

    // Mapping: mnemonic -> mode -> info
    // Mode represented as string for simple parsing logic
    std::map<std::string, std::map<std::string, OpcodeInfo>> m_mnemonics;

    void initTable();
    void add(const std::string& mnem, const std::string& mode, uint8_t op, int sz);
};
