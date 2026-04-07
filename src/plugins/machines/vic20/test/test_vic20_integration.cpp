#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/memory_bus.h"
#include "cpu6502.h"
#include "via6522.h"
#include "vic6560.h"
#include "plugin_loader/main/plugin_loader.h"
#include "cli/main/plugin_command_registry.h"
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
#include "kbd_vic20.h"

// Local wrapper for keyboard device in tests
class TestKeyboardWrapper : public IOHandler, public IKeyboardMatrix {
public:
    TestKeyboardWrapper() { m_kbd = new KbdVic20(); }
    ~TestKeyboardWrapper() { delete m_kbd; }
    const char* name() const override { return "kbd_vic20"; }
    uint32_t baseAddr() const override { return 0; }
    uint32_t addrMask() const override { return 0; }
    bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t) override { return false; }
    void reset() override { m_kbd->clearKeys(); }
    void tick(uint64_t) override {}
    void keyDown(int r, int c) override { m_kbd->keyDown(r, c); }
    void keyUp(int r, int c) override { m_kbd->keyUp(r, c); }
    void clearKeys() override { m_kbd->clearKeys(); }
    bool pressKeyByName(const std::string& n, bool d) override { return m_kbd->pressKeyByName(n, d); }
    IPortDevice* getPort(int i) override { return m_kbd->getPort(i); }
private:
    KbdVic20* m_kbd;
};

// Registry setup — called once; singletons persist across all test cases.
// ---------------------------------------------------------------------------

static bool s_registriesReady = false;
static void ensureRegistriesReady() {
    if (s_registriesReady) return;
    s_registriesReady = true;
    CoreRegistry::instance().registerCore("6502", "NMOS", "open",
        []() -> ICore* { return new MOS6502(); });
    DeviceRegistry::instance().registerDevice("6560",
        []() -> IOHandler* { return new VIC6560("VIC-I", 0x9000); });
    DeviceRegistry::instance().registerDevice("6522",
        []() -> IOHandler* { return new VIA6522("6522", 0); });
    DeviceRegistry::instance().registerDevice("kbd_vic20",
        []() -> IOHandler* { return new TestKeyboardWrapper(); });
    JsonMachineLoader().loadFile("machines/vic20.json");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static FlatMemoryBus* getBus(MachineDescriptor* desc) {
    return static_cast<FlatMemoryBus*>(desc->buses[0].bus);
}

static void destroyDesc(MachineDescriptor* desc) {
    delete desc;
}

// ---------------------------------------------------------------------------
// Test 1: Machine setup — verify the VIC-20 descriptor has the expected shape
// ---------------------------------------------------------------------------

TEST_CASE(vic20_setup) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("vic20");
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "vic20");
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
// Test 2: Execute — write to screen RAM and halt
// ---------------------------------------------------------------------------

TEST_CASE(vic20_execute_screen_write) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("vic20");
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);
    ICore* cpu = desc->cpus[0].cpu;

    // Minimal test program:
    //   $1000  A9 41        LDA #$41
    //   $1002  8D 00 1E     STA $1E00
    //   $1005  4C 05 10     JMP $1005   ; halt (JMP to self)
    bus->write8(0x1000, 0xA9); bus->write8(0x1001, 0x41);
    bus->write8(0x1002, 0x8D); bus->write8(0x1003, 0x00); bus->write8(0x1004, 0x1E);
    bus->write8(0x1005, 0x4C); bus->write8(0x1006, 0x05); bus->write8(0x1007, 0x10);

    // onReset resets all IO and CPU; the KERNAL ROM at $FFFC sets PC to $FD22.
    // Force PC to the test stub directly.
    desc->onReset(*desc);
    cpu->setPc(0x1000);

    // Run until JMP-to-self is detected; guard with iteration cap.
    for (int i = 0; i < 100 && !cpu->isProgramEnd(bus); ++i) {
        desc->schedulerStep(*desc);
    }

    ASSERT(cpu->isProgramEnd(bus));
    ASSERT(bus->read8(0x1E00) == 0x41);

    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 3: Raster frame — VIC-I raster counter advances with CPU cycles
// ---------------------------------------------------------------------------

TEST_CASE(vic20_raster_frame) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("vic20");
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);

    // JMP $1000 (JMP to self) keeps the CPU spinning without halting.
    bus->write8(0x1000, 0x4C); bus->write8(0x1001, 0x00); bus->write8(0x1002, 0x10);
    desc->onReset(*desc);
    desc->cpus[0].cpu->setPc(0x1000);

    // 50 steps × 3 cycles/JMP = 150 cycles.  VIC raster line = 150 / 65 = 2.
    for (int i = 0; i < 50; ++i) desc->schedulerStep(*desc);

    uint8_t rasterLine = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0x9004, &rasterLine);
    ASSERT(ok);
    ASSERT(rasterLine >= 2);

    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 4: VIA1 T1 timer — IFR bit 6 set after underflow
// ---------------------------------------------------------------------------

TEST_CASE(vic20_via_timer) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("vic20");
    ASSERT(desc != nullptr);
    auto* bus = getBus(desc);
    desc->onReset(*desc);

    // Write T1CL latch first, then T1CH to load and start the timer.
    desc->ioRegistry->dispatchWrite(bus, 0x9114, 0x05); // T1CL latch = 5
    desc->ioRegistry->dispatchWrite(bus, 0x9115, 0x00); // T1CH = 0 → starts timer

    desc->ioRegistry->tickAll(50); // 50 cycles — well past the 5-cycle underflow

    uint8_t ifr = 0;
    bool ok = desc->ioRegistry->dispatchRead(bus, 0x911D, &ifr); // VIA1 IFR
    ASSERT(ok);
    ASSERT(ifr & 0x40); // T1 interrupt flag (bit 6) must be set

    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 5: Machine identity — machineId matches the string used by pane filters
// ---------------------------------------------------------------------------

TEST_CASE(vic20_machine_id_pane_match) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("vic20");
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "vic20");
    destroyDesc(desc);
}

// ---------------------------------------------------------------------------
// Test 6: importroms command registered when vice-importer plugin is loaded
// ---------------------------------------------------------------------------

static bool s_importRomsRegistered = false;

static void captureImportRomsCommand(const PluginCommandInfo* info) {
    if (info && std::string(info->name) == "importroms") {
        s_importRomsRegistered = true;
    }
}

TEST_CASE(vic20_importroms_command) {
    s_importRomsRegistered = false;
    PluginLoader::instance().setCommandRegisterFn(captureImportRomsCommand);

    bool loaded = PluginLoader::instance().load("lib/mmemu-plugin-vice-importer.so");
    ASSERT(loaded);
    ASSERT(s_importRomsRegistered);

    PluginLoader::instance().setCommandRegisterFn(nullptr); // restore default stub
}
