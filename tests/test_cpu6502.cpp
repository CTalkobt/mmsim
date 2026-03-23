#include "test_harness.h"
#include "libcore/cpu6502.h"
#include "libmem/memory_bus.h"
#include <cstring>

TEST_CASE(cpu6502_reset) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);

    // Set reset vector
    bus.write8(0xFFFC, 0x00);
    bus.write8(0xFFFD, 0xC0);

    cpu.reset();
    ASSERT(cpu.pc() == 0xC000);
    ASSERT(cpu.regRead(0) == 0); // A
    ASSERT(cpu.regRead(5) == (FLAG_U | FLAG_I)); // P
}

TEST_CASE(cpu6502_basic_ops) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);

    // Program: LDA #$42; STA $10; LDX #$01; STX $11; BRK
    uint8_t prog[] = { 0xA9, 0x42, 0x85, 0x10, 0xA2, 0x01, 0x86, 0x11, 0x00 };
    for (size_t i = 0; i < sizeof(prog); ++i) bus.write8(0xC000 + i, prog[i]);

    cpu.setPc(0xC000);
    
    cpu.step(); // LDA #$42
    ASSERT(cpu.regRead(0) == 0x42);
    ASSERT(cpu.pc() == 0xC002);

    cpu.step(); // STA $10
    ASSERT(bus.read8(0x0010) == 0x42);

    cpu.step(); // LDX #$01
    ASSERT(cpu.regRead(1) == 0x01);

    cpu.step(); // STX $11
    ASSERT(bus.read8(0x0011) == 0x01);
}

TEST_CASE(cpu6502_adc_sbc) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);

    // ADC #$10
    cpu.regWrite(0, 0x20); // A = $20
    cpu.regWrite(5, FLAG_U); // P = $20 (Clear C)
    bus.write8(0x1000, 0x69); // ADC imm
    bus.write8(0x1001, 0x10);
    cpu.setPc(0x1000);
    cpu.step();
    ASSERT(cpu.regRead(0) == 0x30);
    ASSERT(!(cpu.regRead(5) & FLAG_C));

    // SBC #$05
    bus.write8(0x1002, 0xE9); // SBC imm
    bus.write8(0x1003, 0x05);
    cpu.regWrite(5, cpu.regRead(5) | FLAG_C); // Set C for SBC (no borrow)
    cpu.step();
    ASSERT(cpu.regRead(0) == 0x2B);
}

TEST_CASE(cpu6502_branching) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);

    // BNE +$05
    bus.write8(0x1000, 0xD0);
    bus.write8(0x1001, 0x05);
    cpu.setPc(0x1000);
    cpu.regWrite(0, 0x01); // Z = 0
    cpu.regWrite(5, FLAG_U);
    cpu.step();
    ASSERT(cpu.pc() == 0x1007);

    // BNE -$05
    bus.write8(0x1007, 0xD0);
    bus.write8(0x1008, (uint8_t)-5);
    cpu.step();
    ASSERT(cpu.pc() == 0x1004);
}

TEST_CASE(cpu6502_snapshot) {
    MOS6502 cpu;
    cpu.regWrite(0, 0x12);
    cpu.regWrite(1, 0x34);
    cpu.setPc(0xABCD);

    size_t sz = cpu.stateSize();
    std::vector<uint8_t> buf(sz);
    cpu.saveState(buf.data());

    cpu.regWrite(0, 0x00);
    cpu.regWrite(1, 0x00);
    cpu.setPc(0x0000);

    cpu.loadState(buf.data());
    ASSERT(cpu.regRead(0) == 0x12);
    ASSERT(cpu.regRead(1) == 0x34);
    ASSERT(cpu.pc() == 0xABCD);
}
