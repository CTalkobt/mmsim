/*
 * test_tape_roundtrip.cpp
 *
 * For each tape-capable machine (C64, VIC-20, PET 2001) these tests:
 *   1. Write a "Hello World" byte pattern to the machine's BASIC RAM.
 *   2. Arm the datasette for recording and generate a known number of
 *      tape pulses by toggling the cassette-write signal via hardware
 *      registers.
 *   3. Save the recording to a temporary .tap file.
 *   4. Wipe BASIC RAM.
 *   5. Mount the saved .tap and enable the tape motor.
 *   6. Run the datasette and count read-pulse (FLAG / CA1) events.
 *   7. Assert that every recorded pulse was played back.
 *
 * The tests work entirely at the hardware-signal level; no kernal ROMs
 * are required for the core round-trip assertion.
 */

#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/memory_bus.h"
#include "cpu6510.h"
#include "plugin_loader/main/plugin_loader.h"
#include "cli/main/cli_interpreter.h"
#include "plugins/devices/datasette/main/datasette.h"
#include <vector>
#include <string>
#include <cstdio>

static void ensureRegistriesReady() {
    PluginLoader::instance().loadFromDir("./lib");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Datasette* findTape(MachineDescriptor* desc) {
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    for (auto* h : handlers)
        if (std::string(h->name()) == "Tape")
            return static_cast<Datasette*>(h);
    return nullptr;
}

// Count FLAG/CA1 pulse events during playback.
// flagAddr     – address of the ICR / IFR / CRA register
// flagBit      – bitmask of the flag bit
// clearRegAddr – address to read to clear the flag (0 = same as flagAddr)
// clearOnRead  – true if reading flagAddr already clears the flag (CIA style)
static int countPlaybackPulses(MachineDescriptor* desc, IBus* bus,
                               uint32_t flagAddr, uint8_t flagBit,
                               uint32_t clearRegAddr,
                               int expected, int maxCycles)
{
    int count = 0;
    const int step = 10;
    for (int t = 0; t < maxCycles && count < expected; t += step) {
        desc->ioRegistry->tickAll(step);
        uint8_t val = 0;
        if (desc->ioRegistry->dispatchRead(bus, flagAddr, &val) && (val & flagBit)) {
            ++count;
            if (clearRegAddr != flagAddr) {
                // Separate clear register (e.g. read ORA to clear PIA CA1 flag)
                uint8_t dummy = 0;
                desc->ioRegistry->dispatchRead(bus, clearRegAddr, &dummy);
            }
            // CIA ICR is cleared by the read above; VIA IFR needs a write-back
            if (clearRegAddr == flagAddr)
                desc->ioRegistry->dispatchWrite(bus, flagAddr, flagBit);
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// C64 tape round-trip
// ---------------------------------------------------------------------------
// Cassette signals on C64 route through the 6510 I/O port (CPU data bus):
//   Bit 3 (output) = cassette write
//   Bit 5 (output) = cassette motor (active-low: 0 = motor ON)
// Read-pulse arrives on CIA1 FLAG → ICR bit 4 (0x10) at $DC0D.
// CIA ICR is cleared on read; the countPlaybackPulses write-back is harmless.

TEST_CASE(c64_tape_roundtrip) {
    ensureRegistriesReady();

    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create c64");
    ASSERT(ctx.machine != nullptr);

    auto* desc = ctx.machine;
    auto* bus  = static_cast<FlatMemoryBus*>(desc->buses[0].bus);
    cli.processLine("reset");

    Datasette* tape = findTape(desc);
    ASSERT(tape != nullptr);

    // --- Write "Hello World" to BASIC RAM ($0801) ---
    const uint8_t hello[] = "HELLO WORLD";   // 11 bytes + NUL
    for (int i = 0; i < 11; ++i) bus->write8(0x0801 + i, hello[i]);

    // --- Set up 6510 I/O port for cassette output ---
    // The 6510 port is accessed via the CPU data bus (PortBus proxy).
    // DDR  $0000 = $2F : bits 0-3 and 5 are outputs
    // Port $0001 = $17 : motor ON (bit 5=0), write=LOW (bit 3=0) — start LOW
    //                    so the first loop edge (HIGH) creates an immediate transition.
    IBus* cpuBus = desc->cpus[0].cpu->getDataBus();
    cpuBus->write8(0x0000, 0x2F);
    cpuBus->write8(0x0001, 0x17);   // motor ON, write line = LOW
    desc->ioRegistry->tickAll(1);   // let Datasette latch motor ON

    // --- Arm recording (m_lastWriteLevel = LOW) ---
    bool started = tape->startRecord();
    ASSERT(started);

    // Generate NUM_PULSES transitions.  Starting from write=LOW, the first
    // write to $17 (no change) stays LOW; toggling to $1F flips HIGH → pulse.
    // We always begin with hi=true so the very first write is HIGH (transition).
    const int NUM_PULSES   = 20;
    const int PULSE_CYCLES = 300;

    bool high = true;
    for (int i = 0; i < NUM_PULSES; ++i) {
        cpuBus->write8(0x0001, high ? 0x1F : 0x17);  // toggle bit 3
        desc->ioRegistry->tickAll(PULSE_CYCLES);
        high = !high;
    }

    // --- Save recording ---
    tape->stopTapeRecord();
    bool saved = tape->saveTapeRecording("/tmp/c64_hello_world.tap");
    ASSERT(saved);

    // --- Wipe BASIC RAM and verify ---
    for (int i = 0; i < 11; ++i) bus->write8(0x0801 + i, 0x00);
    for (int i = 0; i < 11; ++i) ASSERT(bus->read8(0x0801 + i) == 0x00);

    // --- Mount saved tape and enable motor ---
    bool mounted = tape->mountTape("/tmp/c64_hello_world.tap");
    ASSERT(mounted);

    cpuBus->write8(0x0000, 0x2F);
    cpuBus->write8(0x0001, 0x1F);   // motor ON (bit 5=0)

    // --- Count CIA1 FLAG pulses at $DC0D (bit 4) ---
    // CIA ICR clears on read; pass flagAddr == clearRegAddr to trigger write-back.
    int got = countPlaybackPulses(desc, bus,
        /*flagAddr*/ 0xDC0D, /*flagBit*/ 0x10,
        /*clearRegAddr*/ 0xDC0D,
        NUM_PULSES, /*maxCycles*/ 2000000);

    ASSERT(got >= NUM_PULSES);
}

// ---------------------------------------------------------------------------
// VIC-20 tape round-trip
// ---------------------------------------------------------------------------
// Cassette signals on VIC-20 route through VIA2 (base $9120):
//   PB7 (ORB bit 7, offset 0) = cassette write    [m_pb7Proxy]
//   PB3 (ORB bit 3, offset 0) = cassette motor    [m_pbProxy[3]], active-low
//   DDRB at VIA2 base + 2     = $9122
// Read-pulse arrives on VIA1 CA1 → IFR bit 1 at $911D.
// VIA IFR is NOT cleared on read; clear by writing the bit back.

TEST_CASE(vic20_tape_roundtrip) {
    ensureRegistriesReady();

    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create vic20");
    ASSERT(ctx.machine != nullptr);

    auto* desc = ctx.machine;
    auto* bus  = static_cast<FlatMemoryBus*>(desc->buses[0].bus);
    cli.processLine("reset");

    Datasette* tape = findTape(desc);
    ASSERT(tape != nullptr);

    // --- Write "Hello World" to VIC-20 BASIC RAM ($1001) ---
    const uint8_t hello[] = "HELLO WORLD";
    for (int i = 0; i < 11; ++i) bus->write8(0x1001 + i, hello[i]);

    // --- VIA2 setup ---
    // DDRB $9122 : bits 3 and 7 as outputs (PB3=motor, PB7=write)
    // ORB  $9120 : PB3=0 (motor ON), PB7=0 (write=LOW) — start LOW
    desc->ioRegistry->dispatchWrite(bus, 0x9122, 0x88); // DDRB: bits 3,7 output
    desc->ioRegistry->dispatchWrite(bus, 0x9120, 0x00); // motor ON (PB3=0), write=LOW
    desc->ioRegistry->tickAll(1);

    // --- Arm recording (m_lastWriteLevel = LOW) ---
    bool started = tape->startRecord();
    ASSERT(started);

    const int NUM_PULSES   = 20;
    const int PULSE_CYCLES = 300;

    // Toggle VIA2 PB7 (write line); keep PB3=0 (motor ON) at all times.
    // hi=0x80 (write HIGH), lo=0x00 (write LOW), PB3=0 throughout.
    bool high = true;
    for (int i = 0; i < NUM_PULSES; ++i) {
        desc->ioRegistry->dispatchWrite(bus, 0x9120, high ? 0x80 : 0x00);
        desc->ioRegistry->tickAll(PULSE_CYCLES);
        high = !high;
    }

    // --- Save recording ---
    tape->stopTapeRecord();
    bool saved = tape->saveTapeRecording("/tmp/vic20_hello_world.tap");
    ASSERT(saved);

    // --- Wipe BASIC RAM and verify ---
    for (int i = 0; i < 11; ++i) bus->write8(0x1001 + i, 0x00);
    for (int i = 0; i < 11; ++i) ASSERT(bus->read8(0x1001 + i) == 0x00);

    // --- Mount saved tape and enable motor ---
    bool mounted = tape->mountTape("/tmp/vic20_hello_world.tap");
    ASSERT(mounted);

    desc->ioRegistry->dispatchWrite(bus, 0x9122, 0x08); // DDRB: PB3 output
    desc->ioRegistry->dispatchWrite(bus, 0x9120, 0x00); // PB3=0 (motor ON)

    // --- Count VIA1 CA1 pulses at IFR $911D (bit 1) ---
    // clearRegAddr == flagAddr triggers write-back in the helper.
    int got = countPlaybackPulses(desc, bus,
        /*flagAddr*/ 0x911D, /*flagBit*/ 0x02,
        /*clearRegAddr*/ 0x911D,
        NUM_PULSES, /*maxCycles*/ 2000000);

    ASSERT(got >= NUM_PULSES);
}

// ---------------------------------------------------------------------------
// PET 2001 tape round-trip
// ---------------------------------------------------------------------------
// Cassette signals on PET 2001:
//   Motor  : VIA CA2 output — PCR offset $0C at $E84C
//              PCR bits 3-1 = 110 (0x0C) → CA2 manual low = motor ON
//   Write  : VIA PB7 — ORB at $E840, DDRB at $E842
// Read-pulse: PIA1 CA1 → CRA bit 7.
//   PIA1 base $E810, CRA at offset 1 = $E811.
//   CA1 flag is cleared by reading ORA ($E810).

TEST_CASE(pet2001_tape_roundtrip) {
    ensureRegistriesReady();

    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create pet2001");
    ASSERT(ctx.machine != nullptr);

    auto* desc = ctx.machine;
    auto* bus  = static_cast<FlatMemoryBus*>(desc->buses[0].bus);
    cli.processLine("reset");

    Datasette* tape = findTape(desc);
    ASSERT(tape != nullptr);

    // --- Write "Hello World" to PET BASIC RAM ($0401) ---
    const uint8_t hello[] = "HELLO WORLD";
    for (int i = 0; i < 11; ++i) bus->write8(0x0401 + i, hello[i]);

    // --- VIA setup ---
    // DDRB $E842 : bit 7 output (0x80)
    // PCR  $E84C : CA2 manual low = motor ON (bits 3-1 = 110 → 0x0C)
    // ORB  $E840 : PB7=0 (write=LOW) — start LOW
    desc->ioRegistry->dispatchWrite(bus, 0xE842, 0x80); // DDRB: PB7 output
    desc->ioRegistry->dispatchWrite(bus, 0xE84C, 0x0C); // PCR: CA2 manual low = motor ON
    desc->ioRegistry->dispatchWrite(bus, 0xE840, 0x00); // ORB: write=LOW
    desc->ioRegistry->tickAll(1);

    // --- Arm recording (m_lastWriteLevel = LOW) ---
    bool started = tape->startRecord();
    ASSERT(started);

    const int NUM_PULSES   = 20;
    const int PULSE_CYCLES = 300;

    // Toggle VIA PB7 (write line): hi=0x80, lo=0x00
    bool high = true;
    for (int i = 0; i < NUM_PULSES; ++i) {
        desc->ioRegistry->dispatchWrite(bus, 0xE840, high ? 0x80 : 0x00);
        desc->ioRegistry->tickAll(PULSE_CYCLES);
        high = !high;
    }

    // --- Save recording ---
    tape->stopTapeRecord();
    bool saved = tape->saveTapeRecording("/tmp/pet2001_hello_world.tap");
    ASSERT(saved);

    // --- Wipe BASIC RAM and verify ---
    for (int i = 0; i < 11; ++i) bus->write8(0x0401 + i, 0x00);
    for (int i = 0; i < 11; ++i) ASSERT(bus->read8(0x0401 + i) == 0x00);

    // --- Mount saved tape and re-enable motor ---
    bool mounted = tape->mountTape("/tmp/pet2001_hello_world.tap");
    ASSERT(mounted);

    desc->ioRegistry->dispatchWrite(bus, 0xE84C, 0x0C); // CA2 motor ON

    // --- Count PIA1 CA1 pulses: CRA bit 7 at $E811 ---
    // Cleared by reading ORA ($E810); use separate clearRegAddr.
    int got = countPlaybackPulses(desc, bus,
        /*flagAddr*/ 0xE811, /*flagBit*/ 0x80,
        /*clearRegAddr*/ 0xE810,
        NUM_PULSES, /*maxCycles*/ 2000000);

    ASSERT(got >= NUM_PULSES);
}
