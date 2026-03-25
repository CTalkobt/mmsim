#pragma once

#include "libtoolchain/main/idisasm.h"
#include "cpu6502.h"
#include "libtoolchain/main/symbol_table.h"

class Disassembler6502 : public IDisassembler {
public:
    Disassembler6502(SymbolTable* symbols = nullptr);

    const char* isaName() const override { return "6502"; }

    int disasmOne(IBus* bus, uint32_t addr, char* buf, int bufsz) override;
    int disasmEntry(IBus* bus, uint32_t addr, DisasmEntry& entry) override;

private:
    MOS6502 m_cpu;
    SymbolTable* m_symbols;

    void resolveSymbols(DisasmEntry& entry);
};
