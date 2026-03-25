#include "test_harness.h"
#include "plugins/6502/disassembler_6502.h"
#include "libmem/memory_bus.h"
#include <cstring>

TEST_CASE(disasm6502_basic) {
    FlatMemoryBus bus("system", 16);
    // LDA #$42; STA $1234; JSR $C000; RTS
    uint8_t prog[] = { 0xA9, 0x42, 0x8D, 0x34, 0x12, 0x20, 0x00, 0xC0, 0x60 };
    for (size_t i = 0; i < sizeof(prog); ++i) bus.write8(0x1000 + i, prog[i]);

    Disassembler6502 disasm;
    char buf[64];
    int bytes;

    bytes = disasm.disasmOne(&bus, 0x1000, buf, sizeof(buf));
    ASSERT(bytes == 2);
    ASSERT(std::string(buf).find("LDA") != std::string::npos);
    ASSERT(std::string(buf).find("#$42") != std::string::npos);

    bytes = disasm.disasmOne(&bus, 0x1002, buf, sizeof(buf));
    ASSERT(bytes == 3);
    ASSERT(std::string(buf).find("STA") != std::string::npos);
    ASSERT(std::string(buf).find("$1234") != std::string::npos);
}

TEST_CASE(disasm6502_symbols) {
    FlatMemoryBus bus("system", 16);
    bus.write8(0x1000, 0x4C); // JMP $2000
    bus.write8(0x1001, 0x00);
    bus.write8(0x1002, 0x20);

    SymbolTable symbols;
    symbols.addSymbol(0x2000, "target_label");

    Disassembler6502 disasm(&symbols);
    char buf[64];
    disasm.disasmOne(&bus, 0x1000, buf, sizeof(buf));
    
    ASSERT(std::string(buf).find("target_label") != std::string::npos);
}

TEST_CASE(disasm6502_entry) {
    FlatMemoryBus bus("system", 16);
    bus.write8(0x1000, 0x20); // JSR $C000
    bus.write8(0x1001, 0x00);
    bus.write8(0x1002, 0xC0);

    Disassembler6502 disasm;
    DisasmEntry entry;
    disasm.disasmEntry(&bus, 0x1000, entry);

    ASSERT(entry.mnemonic == "JSR");
    ASSERT(entry.isCall == true);
    ASSERT(entry.targetAddr == 0xC000);
}
