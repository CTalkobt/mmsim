#include "test_harness.h"
#include "libdebug/main/debug_context.h"
#include "plugins/6502/main/cpu6502.h"
#include "libmem/main/memory_bus.h"

TEST_CASE(debug_breakpoints) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);
    
    DebugContext dbg(&cpu, &bus);
    cpu.observer = &dbg;
    bus.observer = &dbg;

    // Program: NOP ($EA); STA $10 ($85 $10)
    bus.write8(0x1000, 0xEA);
    bus.write8(0x1001, 0x85);
    bus.write8(0x1002, 0x10);
    cpu.setPc(0x1000);
    cpu.regWrite(0, 0x42); // A = $42

    int bpId = dbg.breakpoints().add(0x1001, BreakpointType::EXEC);
    int wpId = dbg.breakpoints().add(0x0010, BreakpointType::WRITE_WATCH);
    int rpId = dbg.breakpoints().add(0x0011, BreakpointType::READ_WATCH);

    cpu.step(); // Execute NOP
    ASSERT(dbg.breakpoints().breakpoints()[0].hitCount == 0);

    cpu.step(); // Execute STA $10
    ASSERT(dbg.breakpoints().breakpoints()[0].hitCount == 1); // EXEC bp hit
    ASSERT(dbg.breakpoints().breakpoints()[1].hitCount == 1); // WRITE wp hit

    // Test write with same value
    bus.write8(0x0010, 0x42); 
    ASSERT(dbg.breakpoints().breakpoints()[1].hitCount == 2); // Should hit again

    // Test read-watch
    bus.read8(0x0011);
    ASSERT(dbg.breakpoints().breakpoints()[2].hitCount == 1); // READ wp hit
}

TEST_CASE(debug_trace) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);
    
    DebugContext dbg(&cpu, &bus);
    cpu.observer = &dbg;

    bus.write8(0x1000, 0xA9); // LDA #$42
    bus.write8(0x1001, 0x42);
    cpu.setPc(0x1000);

    cpu.step();

    ASSERT(dbg.trace().size() == 1);
    ASSERT(dbg.trace().at(0).addr == 0x1000);
    ASSERT(dbg.trace().at(0).regs.at("A") == 0x42);
}

TEST_CASE(debug_snapshots) {
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    cpu.setDataBus(&bus);
    
    DebugContext dbg(&cpu, &bus);

    cpu.regWrite(0, 0xAA);
    bus.write8(0x2000, 0x55);
    
    int snapIdx = dbg.saveSnapshot("test");
    
    cpu.regWrite(0, 0xBB);
    bus.write8(0x2000, 0x66);
    
    dbg.restoreSnapshot(snapIdx);
    
    ASSERT(cpu.regRead(0) == 0xAA);
    ASSERT(bus.read8(0x2000) == 0x55);
}
