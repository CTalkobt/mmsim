#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/memory_bus.h"
#include "cpu6510.h"
#include "c64_pla.h"
#include "cia6526.h"
#include "vic2.h"
#include "sid6581.h"
#include <vector>
#include <string>

// Linked directly into the test binary — no dlopen for core machine components.
MachineDescriptor* createMachineC64();

// ---------------------------------------------------------------------------
// Registry setup — called once; singletons persist across all test cases.
// ---------------------------------------------------------------------------

static bool s_registriesReady = false;
static void ensureRegistriesReady() {
    if (s_registriesReady) return;
    s_registriesReady = true;
    CoreRegistry::instance().registerCore("6510", "MOS6510", "open",
        []() -> ICore* { return new MOS6510(); });
    DeviceRegistry::instance().registerDevice("6567",
        []() -> IOHandler* { return new VIC2("VIC-II", 0xD000); });
    DeviceRegistry::instance().registerDevice("6581",
        []() -> IOHandler* { return new SID6581("SID", 0xD400); });
    DeviceRegistry::instance().registerDevice("6526",
        []() -> IOHandler* { return new CIA6526("CIA", 0); });
    MachineRegistry::instance().registerMachine("c64", createMachineC64);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static FlatMemoryBus* getBus(MachineDescriptor* desc) {
    return static_cast<FlatMemoryBus*>(desc->buses[0].bus);
}

static void destroyDesc(MachineDescriptor* desc) {
    if (!desc) return;
    if (desc->ioRegistry) {
        std::vector<IOHandler*> handlers;
        desc->ioRegistry->enumerate(handlers);
        delete desc->ioRegistry;
        for (auto* h : handlers) delete h;
    }
    for (auto& slot : desc->cpus) delete slot.cpu;
    for (auto& slot : desc->buses) delete slot.bus;
    for (auto* rom : desc->roms) delete[] rom;
    delete desc;
}

// ---------------------------------------------------------------------------
// Test 1: Machine setup — verify the C64 descriptor has the expected shape
// ---------------------------------------------------------------------------

TEST_CASE(c64_setup) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == std::string("c64"));
    ASSERT(!desc->cpus.empty());
    ASSERT(desc->cpus[0].cpu != nullptr);
    ASSERT(!desc->buses.empty());
    ASSERT(desc->buses[0].bus != nullptr);
    ASSERT(desc->ioRegistry != nullptr);
    ASSERT(desc->schedulerStep != nullptr);
    ASSERT(desc->onReset != nullptr);
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 2: CPU variant — the C64 machine uses a MOS 6510 core
// ---------------------------------------------------------------------------

TEST_CASE(c64_cpu_is_6510) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
    ICore* cpu = desc->cpus[0].cpu;
    ASSERT(cpu != nullptr);
    ASSERT(std::string(cpu->variantName()) == "MOS 6510");
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 3: Execute — write a value to RAM, read it back
// ---------------------------------------------------------------------------

TEST_CASE(c64_execute_ram_write) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
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

    desc->onReset(*desc);
    cpu->setPc(0x2000);

    for (int i = 0; i < 100 && !cpu->isProgramEnd(bus); ++i)
        desc->schedulerStep(*desc);

    ASSERT(cpu->isProgramEnd(bus));
    ASSERT(bus->read8(0x0400) == 0x55);
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 4: 6510 I/O port — DDR and DATA registers at $00/$01
// ---------------------------------------------------------------------------

TEST_CASE(c64_6510_io_port) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
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

    desc->onReset(*desc);
    cpu->setPc(0x3000);

    for (int i = 0; i < 200 && !cpu->isProgramEnd(bus); ++i)
        desc->schedulerStep(*desc);

    ASSERT(cpu->isProgramEnd(bus));

    // Verify DDR and DATA readable via bus (PortBus intercepts $00/$01).
    MOS6510* cpu6510 = dynamic_cast<MOS6510*>(cpu);
    ASSERT(cpu6510 != nullptr);
    ASSERT(cpu6510->portDDR()  == 0x07);
    ASSERT(cpu6510->portData() == 0x37);
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 5: CIA1 timer — ICR_TA fires after counter underflow
// ---------------------------------------------------------------------------

TEST_CASE(c64_cia1_timer) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);
    desc->onReset(*desc);

    // Load latch, then start timer in one-shot mode.
    desc->ioRegistry->dispatchWrite(bus, 0xDC04, 0x05); // TALO latch = 5
    desc->ioRegistry->dispatchWrite(bus, 0xDC05, 0x00); // TAHI latch = 0
    desc->ioRegistry->dispatchWrite(bus, 0xDC0E, 0x09); // CRA: START | ONESHOT

    desc->ioRegistry->tickAll(50); // well past 5-cycle underflow

    uint8_t icr = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0xDC0D, &icr); // CIA1 ICR
    ASSERT(ok);
    ASSERT(icr & 0x01); // ICR_TA (bit 0) must be set
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 6: PLA banking — HIRAM=1 reveals KERNAL ROM at $E000
//         (when roms/c64/kernal.bin is absent the buffer remains zeroed;
//          we just verify the PLA routes the read to the PLA handler)
// ---------------------------------------------------------------------------

TEST_CASE(c64_pla_kernal_routing) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);
    desc->onReset(*desc);

    // After reset, 6510 DATA=$3F DDR=$00 → all bits float high (pull-ups).
    // Effective port: (~$00 & $3F) = $3F → HIRAM=1, LORAM=1, CHAREN=1.
    // But DDR=0 means all inputs → port floats to pull-up $3F.
    // On real C64 this maps KERNAL ROM at $E000.
    //
    // Force HIRAM=1 by writing DDR=$07 (bits 0-2 output) and DATA=$37 via bus.
    bus->write8(0x0000, 0x07); // DDR  — bits 0-2 output
    bus->write8(0x0001, 0x37); // DATA — LORAM=1, HIRAM=1, CHAREN=1

    // The PLA should now intercept reads from $E000 and return from kernalRom[].
    // Without ROM files the buffer is zero; dispatchRead should return true
    // (the PLA handler claims the address).
    uint8_t val = 0xAB; // sentinel — will be overwritten if PLA responds
    bool claimed = desc->ioRegistry->dispatchRead(bus, 0xE000, &val);
    ASSERT(claimed);        // PLA must have handled the read
    ASSERT(val == 0x00);    // ROM buffer is zero (no ROM file in test env)
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 7: VIC-II raster counter advances with CPU cycles
// ---------------------------------------------------------------------------

TEST_CASE(c64_vic2_raster_advance) {
    ensureRegistriesReady();
    auto* desc = createMachineC64();
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);

    // JMP $4000 (self) keeps CPU spinning without halting.
    bus->write8(0x4000, 0x4C);
    bus->write8(0x4001, 0x00);
    bus->write8(0x4002, 0x40);
    desc->onReset(*desc);
    desc->cpus[0].cpu->setPc(0x4000);

    // 65 cycles per raster line (NTSC 6567); run at least 2 lines.
    for (int i = 0; i < 200; ++i) desc->schedulerStep(*desc);

    uint8_t raster = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0xD012, &raster);
    ASSERT(ok);
    ASSERT(raster >= 1); // at least one raster line completed
    destroyDesc(desc);
}
