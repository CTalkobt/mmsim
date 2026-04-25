#pragma once

#include "libtoolchain/main/idisasm.h"
#include "cpu45gs02.h"
#include "libtoolchain/main/symbol_table.h"

class Disassembler45GS02 : public IDisassembler {
public:
    Disassembler45GS02(SymbolTable* symbols = nullptr) : m_symbols(symbols) {}

    const char* isaName() const override { return "45GS02"; }

    int disasmOne(IBus* bus, uint32_t addr, char* buf, int bufsz) override {
        DisasmEntry entry;
        int bytes = disasmEntry(bus, addr, entry);
        std::snprintf(buf, bufsz, "%s", entry.complete.c_str());
        return bytes;
    }

    int disasmEntry(IBus* bus, uint32_t addr, DisasmEntry& entry) override {
        int bytes = m_cpu.disassembleEntry(bus, addr, &entry);
        // Symbol resolution could be added here later if needed
        return bytes;
    }

    void setSymbolTable(SymbolTable* symbols) override { m_symbols = symbols; }

private:
    MOS45GS02 m_cpu;
    SymbolTable* m_symbols;
};
