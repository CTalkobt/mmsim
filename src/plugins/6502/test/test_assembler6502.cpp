#include "test_harness.h"
#include "plugins/6502/main/assembler_6502.h"
#include "libmem/main/memory_bus.h"
#include <cstring>

TEST_CASE(assembler6502_basic) {
    Assembler6502 assem;
    uint8_t buf[16];
    int sz;

    // Immediate
    sz = assem.assembleLine("LDA #$42", buf, sizeof(buf));
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0xA9);
    ASSERT(buf[1] == 0x42);

    // Zero Page
    sz = assem.assembleLine("STA $80", buf, sizeof(buf));
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0x85);
    ASSERT(buf[1] == 0x80);

    // Absolute
    sz = assem.assembleLine("JMP $1234", buf, sizeof(buf));
    ASSERT(sz == 3);
    ASSERT(buf[0] == 0x4C);
    ASSERT(buf[1] == 0x34);
    ASSERT(buf[2] == 0x12);

    // Implicit
    sz = assem.assembleLine("INX", buf, sizeof(buf));
    ASSERT(sz == 1);
    ASSERT(buf[0] == 0xE8);
}

TEST_CASE(assembler6502_addressing_modes) {
    Assembler6502 assem;
    uint8_t buf[16];
    int sz;

    // Accumulator
    sz = assem.assembleLine("ASL A", buf, sizeof(buf));
    ASSERT(sz == 1);
    ASSERT(buf[0] == 0x0A);

    // Indexed X
    sz = assem.assembleLine("LDA $1234,X", buf, sizeof(buf));
    ASSERT(sz == 3);
    ASSERT(buf[0] == 0xBD);

    // Indexed Y
    sz = assem.assembleLine("LDX $1234,Y", buf, sizeof(buf));
    ASSERT(sz == 3);
    ASSERT(buf[0] == 0xBE);

    // Indirect
    sz = assem.assembleLine("JMP ($1234)", buf, sizeof(buf));
    ASSERT(sz == 3);
    ASSERT(buf[0] == 0x6C);

    // Indexed Indirect (X)
    sz = assem.assembleLine("LDA ($80,X)", buf, sizeof(buf));
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0xA1);

    // Indirect Indexed (Y)
    sz = assem.assembleLine("LDA ($80),Y", buf, sizeof(buf));
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0xB1);
}

TEST_CASE(assembler6502_relative) {
    Assembler6502 assem;
    uint8_t buf[16];
    int sz;

    // Forward branch
    // BEQ $100A from $1000. PC after BEQ is $1002. Offset = 100A - 1002 = 8.
    sz = assem.assembleLine("BEQ $100A", buf, sizeof(buf), 0x1000);
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0xF0);
    ASSERT(buf[1] == 0x08);

    // Backward branch
    // BNE $1000 from $1006. PC after BNE is $1008. Offset = 1000 - 1008 = -8 (0xF8).
    sz = assem.assembleLine("BNE $1000", buf, sizeof(buf), 0x1006);
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0xD0);
    ASSERT(buf[1] == 0xF8);
}

TEST_CASE(assembler6502_illegals) {
    Assembler6502 assem;
    uint8_t buf[16];
    int sz;

    // SLO ZP
    sz = assem.assembleLine("SLO $10", buf, sizeof(buf));
    ASSERT(sz == 2);
    ASSERT(buf[0] == 0x07);

    // LAX ABS
    sz = assem.assembleLine("LAX $1234", buf, sizeof(buf));
    ASSERT(sz == 3);
    ASSERT(buf[0] == 0xAF);
}
