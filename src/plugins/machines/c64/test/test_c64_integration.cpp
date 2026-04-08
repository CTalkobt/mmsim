#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/memory_bus.h"
#include "cpu6510.h"
#include "c64_pla.h"
#include "cia6526.h"
#include "vic2.h"
#include "sid6581.h"
#include "kbd_c64.h"
#include "plugin_loader/main/plugin_loader.h"
#include "cli/main/cli_interpreter.h"
#include <vector>
#include <string>
#include <cstring>

// ---------------------------------------------------------------------------
// Registry setup — called once; singletons persist across all test cases.
// ---------------------------------------------------------------------------

static void ensureRegistriesReady() {
    PluginLoader::instance().loadFromDir("./lib");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static FlatMemoryBus* getBus(MachineDescriptor* desc) {
    if (!desc || desc->buses.empty()) return nullptr;
    return static_cast<FlatMemoryBus*>(desc->buses[0].bus);
}

// ---------------------------------------------------------------------------
// Test 1: Machine setup — verify the C64 descriptor has the expected shape
// ---------------------------------------------------------------------------

TEST_CASE(c64_setup) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    ASSERT(ctx.machine->machineId == std::string("c64"));
    ASSERT(!ctx.machine->cpus.empty());
    ASSERT(ctx.machine->cpus[0].cpu != nullptr);
    ASSERT(!ctx.machine->buses.empty());
    ASSERT(ctx.machine->buses[0].bus != nullptr);
    ASSERT(ctx.machine->ioRegistry != nullptr);
    ASSERT(ctx.machine->schedulerStep != nullptr);
    ASSERT(ctx.machine->onReset != nullptr);
    
}

// ---------------------------------------------------------------------------
// Test 2: CPU variant — the C64 machine uses a MOS 6510 core
// ---------------------------------------------------------------------------

TEST_CASE(c64_cpu_is_6510) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    ICore* cpu = ctx.machine->cpus[0].cpu;
    ASSERT(cpu != nullptr);
    ASSERT(std::string(cpu->variantName()) == "MOS 6510");
    
}

// ---------------------------------------------------------------------------
// Test 3: Execute — write a value to RAM, read it back
// ---------------------------------------------------------------------------

TEST_CASE(c64_execute_ram_write) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    ICore* cpu = desc->cpus[0].cpu;

    // $0400–$07FF is screen RAM on a default C64; it's also plain RAM
    // when no ROM is mapped (no ROM files in test env).
    //   $2000  A9 55   LDA #$55
    //   $2002  8D 00 04  STA $0400
    //   $2005  4C 05 20  JMP $2005   ; halt
    bus->write8(0x2000, 0xA9); bus->write8(0x2001, 0x55);
    bus->write8(0x2002, 0x8D); bus->write8(0x2003, 0x00); bus->write8(0x2004, 0x04);
    bus->write8(0x2005, 0x4C); bus->write8(0x2006, 0x05); bus->write8(0x2007, 0x20);

    cli.processLine("reset");
    cpu->setPc(0x2000);

    for (int i = 0; i < 100 && !cpu->isProgramEnd(bus); ++i)
        desc->schedulerStep(*desc);

    ASSERT(cpu->isProgramEnd(bus));
    ASSERT(bus->read8(0x0400) == 0x55);
    
}

// ---------------------------------------------------------------------------
// Test 4: 6510 I/O port — DDR and DATA registers at $00/$01
// ---------------------------------------------------------------------------

TEST_CASE(c64_6510_io_port) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    ICore* cpu = desc->cpus[0].cpu;

    // Program: write $07 to DDR ($00), write $37 to DATA ($01)
    //   $3000  A9 07   LDA #$07
    //   $3002  85 00   STA $00      ; DDR = $07 (bits 0-2 output)
    //   $3004  A9 37   LDA #$37
    //   $3006  85 01   STA $01      ; DATA = $37 (LORAM=1, HIRAM=1, CHAREN=1)
    //   $3008  4C 08 30  JMP $3008
    bus->write8(0x3000, 0xA9); bus->write8(0x3001, 0x07);
    bus->write8(0x3002, 0x85); bus->write8(0x3003, 0x00);
    bus->write8(0x3004, 0xA9); bus->write8(0x3005, 0x37);
    bus->write8(0x3006, 0x85); bus->write8(0x3007, 0x01);
    bus->write8(0x3008, 0x4C); bus->write8(0x3009, 0x08); bus->write8(0x300A, 0x30);

    cli.processLine("reset");
    cpu->setPc(0x3000);

    for (int i = 0; i < 200 && !cpu->isProgramEnd(bus); ++i)
        desc->schedulerStep(*desc);

    ASSERT(cpu->isProgramEnd(bus));

    // Verify DDR and DATA readable via bus (PortBus intercepts $00/$01).
    MOS6510* cpu6510 = dynamic_cast<MOS6510*>(cpu);
    ASSERT(cpu6510 != nullptr);
    ASSERT(cpu6510->portDDR()  == 0x07);
    ASSERT(cpu6510->portData() == 0x37);
    
}

// ---------------------------------------------------------------------------
// Test 5: CIA1 timer — ICR_TA fires after counter underflow
// ---------------------------------------------------------------------------

TEST_CASE(c64_cia1_timer) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    cli.processLine("reset");

    // Load latch, then start timer in one-shot mode.
    desc->ioRegistry->dispatchWrite(bus, 0xDC04, 0x05); // TALO latch = 5
    desc->ioRegistry->dispatchWrite(bus, 0xDC05, 0x00); // TAHI latch = 0
    desc->ioRegistry->dispatchWrite(bus, 0xDC0E, 0x09); // CRA: START | ONESHOT

    desc->ioRegistry->tickAll(50); // well past 5-cycle underflow

    uint8_t icr = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0xDC0D, &icr); // CIA1 ICR
    ASSERT(ok);
    ASSERT(icr & 0x01); // ICR_TA (bit 0) must be set
    
}

// ---------------------------------------------------------------------------
// Test 6: PLA banking — HIRAM=1 reveals KERNAL ROM at $E000
//         (when roms/c64/kernal.bin is absent the buffer remains zeroed;
//          we just verify the PLA routes the read to the PLA handler)
// ---------------------------------------------------------------------------

TEST_CASE(c64_pla_kernal_routing) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    cli.processLine("reset");

    // After reset, 6510 DATA=$3F DDR=$00 → all bits float high (pull-ups).
    // Effective port: (~$00 & $3F) = $3F → HIRAM=1, LORAM=1, CHAREN=1.
    // But DDR=0 means all inputs → port floats to pull-up $3F.
    // On real C64 this maps KERNAL ROM at $E000.
    //
    // Force HIRAM=1 by writing DDR=$07 (bits 0-2 output) and DATA=$37 via bus.
    bus->write8(0x0000, 0x07); // DDR  — bits 0-2 output
    bus->write8(0x0001, 0x37); // DATA — LORAM=1, HIRAM=1, CHAREN=1

    // The PLA should now intercept reads from $E000 and return from kernalRom[].
    // The byte value depends on whether real ROM files are present; just verify
    // the PLA handler claimed the address (returned true).
    uint8_t val = 0xAB; // sentinel — will be overwritten if PLA responds
    bool claimed = desc->ioRegistry->dispatchRead(bus, 0xE000, &val);
    ASSERT(claimed);        // PLA must have handled the read
    ASSERT(val != 0xAB);    // val must have been written by the PLA handler
    
}

// ---------------------------------------------------------------------------
// Test 7: VIC-II raster counter advances with CPU cycles
// ---------------------------------------------------------------------------

TEST_CASE(c64_vic2_raster_advance) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);

    // JMP $4000 (self) keeps CPU spinning without halting.
    bus->write8(0x4000, 0x4C);
    bus->write8(0x4001, 0x00);
    bus->write8(0x4002, 0x40);
    cli.processLine("reset");
    desc->cpus[0].cpu->setPc(0x4000);

    // 65 cycles per raster line (NTSC 6567); run at least 2 lines.
    for (int i = 0; i < 200; ++i) desc->schedulerStep(*desc);

    uint8_t raster = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0xD012, &raster);
    ASSERT(ok);
    ASSERT(raster >= 1); // at least one raster line completed
    
}

// ---------------------------------------------------------------------------
// Test 8: Color RAM — write and read back; upper nibble reads as 0xF, lower
//         4 bits hold the stored value.
// ---------------------------------------------------------------------------

TEST_CASE(c64_color_ram) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    cli.processLine("reset");

    // Color RAM is at $D800–$DBFF (1 KB, 4-bit cells).
    // Write a sentinel value; the handler stores only the lower nibble.
    bool wok = desc->ioRegistry->dispatchWrite(bus, 0xD800, 0xA5); // store 0x05
    ASSERT(wok);

    uint8_t val = 0;
    bool rok = desc->ioRegistry->dispatchRead(bus, 0xD800, &val);
    ASSERT(rok);
    ASSERT((val & 0x0F) == 0x05);  // lower nibble preserved
    ASSERT((val & 0xF0) == 0xF0);  // upper nibble always reads 1s

    // Verify last cell of color RAM is also accessible.
    bool wok2 = desc->ioRegistry->dispatchWrite(bus, 0xDBFF, 0x3C); // store 0x0C
    ASSERT(wok2);

    uint8_t val2 = 0;
    bool rok2 = desc->ioRegistry->dispatchRead(bus, 0xDBFF, &val2);
    ASSERT(rok2);
    ASSERT((val2 & 0x0F) == 0x0C);
    ASSERT((val2 & 0xF0) == 0xF0);

}

// ---------------------------------------------------------------------------
// Test 9: Keyboard matrix — press and scan via CIA1 Port A/B.
//         The C64 KERNAL scans by driving PA (column) low and reading PB (rows).
// ---------------------------------------------------------------------------

TEST_CASE(c64_keyboard_scan) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);
    cli.processLine("reset");
    ASSERT(desc->onKey != nullptr);

    // Press "a" — PA1 column (PB2 row): keymap {col=1, row=2}.
    bool found = desc->onKey("a", true);
    ASSERT(found);

    // Simulate KERNAL scan: set DDRA=0xFF (all outputs), write col-select to PA.
    desc->ioRegistry->dispatchWrite(bus, 0xDC02, 0xFF); // DDRA = all outputs
    desc->ioRegistry->dispatchWrite(bus, 0xDC00, ~0x02 & 0xFF); // select PA1 low

    // Read PB ($DC01): PB2 (bit 2) must be low (key pressed).
    uint8_t pb = 0xFF;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0xDC01, &pb);
    ASSERT(ok);
    ASSERT(!(pb & 0x04)); // PB2 = bit 2 must be 0 (pressed)

    // All other columns: select a column where "a" is NOT present → all rows high.
    desc->ioRegistry->dispatchWrite(bus, 0xDC00, ~0x01 & 0xFF); // select PA0
    uint8_t pb0 = 0;
    desc->ioRegistry->dispatchRead(bus, 0xDC01, &pb0);
    ASSERT(pb0 == 0xFF); // no key in PA0 column pressed

}

// ---------------------------------------------------------------------------
// Test 10: VIC-II Character Map visibility — verify the VIC-II can see its
//          character ROM even when the CPU has banked it out for I/O.
// ---------------------------------------------------------------------------

TEST_CASE(c64_vic2_character_map_visibility) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");

    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = getBus(desc);

    // Find the VIC2 instance in the IO registry
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    VIC2* vic2 = nullptr;
    for (auto* h : handlers) {
        if (std::string(h->name()) == "VIC-II") {
            vic2 = static_cast<VIC2*>(h);
            break;
        }
    }
    ASSERT(vic2 != nullptr);

    // 1. Setup mock Character ROM data
    uint8_t mockCharRom[4096];
    std::memset(mockCharRom, 0, sizeof(mockCharRom));
    mockCharRom[0x0123] = 0x42; // Unique value at offset $123
    vic2->setCharRom(mockCharRom, sizeof(mockCharRom));

    // 2. Configure CPU to see I/O at $D000 (bank out Char ROM)
    // 6510 Port: bit 2 (CHAREN) = 1 for I/O, 0 for Char ROM.
    bus->write8(0x0000, 0x07); // DDR: bits 0-2 output
    bus->write8(0x0001, 0x37); // DATA: %00110111 (LORAM=1, HIRAM=1, CHAREN=1)

    // Verify CPU sees I/O at $D020 (border color register)
    // Default border color is $0E; bits 4-7 read as 1.
    ASSERT(bus->read8(0xD020) == 0xFE);

    // 3. Verify VIC-II still sees Character ROM via DMA
    // In VIC bank 0 ($0000-$3FFF), Char ROM is shadowed at $1000-$1FFF.
    // Offset $1123 in bank 0 maps to Char ROM offset $0123.
    ASSERT(vic2->dmaPeek(0x1123) == 0x42);

    // 4. Test Bank 2 ($8000-$BFFF) — Char ROM shadowed at $9000-$9FFF
    // Switch to Bank 2: CIA2 Port A bits 0-1 = %01 (inverted -> %10 = 2)
    // CIA2 Port A is at $DD00.
    desc->ioRegistry->dispatchWrite(bus, 0xDD02, 0x03); // DDRA bits 0-1 output
    desc->ioRegistry->dispatchWrite(bus, 0xDD00, 0x01); // PRA bits 0-1 = %01
    
    // Offset $1123 in bank 2 ($8000 + $1123 = $9123) maps to Char ROM offset $0123.
    ASSERT(vic2->dmaPeek(0x1123) == 0x42);

    // 5. Test Bank 1 ($4000-$7FFF) — Char ROM NOT shadowed here (sees bus directly)
    desc->ioRegistry->dispatchWrite(bus, 0xDD00, 0x02); // effective %10 -> bankBits %01 (1)
    
    // Fill RAM at $4000 + $1123 = $5123 with a different value
    bus->write8(0x5123, 0x99);
    ASSERT(vic2->dmaPeek(0x1123) == 0x99); // Should NOT see Char ROM

}
