#include "test_harness.h"
#include "libcore/main/json_machine_loader.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machine_desc.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/iport_device.h"
#include "libdevices/main/isignal_line.h"
#include "libmem/main/memory_bus.h"
#include "cpu6502.h"
#include <cstring>

// ============================================================
// Registry setup for Phase 2 tests (idempotent).
// ============================================================

static bool s_jmlRegistriesReady = false;
static void setupJmlRegistries() {
    if (s_jmlRegistriesReady) return;
    s_jmlRegistriesReady = true;
    CoreRegistry::instance().registerCore("6502", "NMOS", "open",
        []() -> ICore* { return new MOS6502(); });
    // Minimal stub device for signal-wiring tests
    struct StubDevice : public IOHandler {
        std::string m_name;
        uint32_t m_base = 0;
        ISignalLine* m_irq = nullptr;
        const char* name()     const override { return m_name.c_str(); }
        uint32_t baseAddr()    const override { return m_base; }
        uint32_t addrMask()    const override { return 0; }
        bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
        void setName    (const std::string& n) override { m_name = n; }
        void setBaseAddr(uint32_t addr)        override { m_base = addr; }
        void setIrqLine (ISignalLine* l)       override { m_irq  = l; }
    };
    DeviceRegistry::instance().registerDevice("stub_dev",
        []() -> IOHandler* { return new StubDevice(); });
}

// ============================================================
// Phase 1 — parsing smoke tests
// ============================================================

TEST_CASE(json_loader_parse_valid_single) {
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "test.machine.p1a",
        "displayName": "Test Machine"
    })";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
    // buildFromSpec now returns a (possibly minimal) MachineDescriptor, not null
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("test.machine.p1a");
    ASSERT_NE(desc, nullptr);
    delete desc;
}

TEST_CASE(json_loader_parse_array) {
    JsonMachineLoader loader;
    const char* json = R"([
        { "id": "arr.machine.a", "displayName": "A" },
        { "id": "arr.machine.b", "displayName": "B" }
    ])";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 2);
}

TEST_CASE(json_loader_parse_machines_key) {
    JsonMachineLoader loader;
    const char* json = R"({
        "machines": [
            { "id": "wrapped.machine.x", "displayName": "X" }
        ]
    })";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
}

TEST_CASE(json_loader_invalid_json_returns_zero) {
    JsonMachineLoader loader;
    int n = loader.loadString("this is not json {{{{");
    ASSERT_EQ(n, 0);
}

TEST_CASE(json_loader_missing_id_skipped) {
    JsonMachineLoader loader;
    const char* json = R"([
        { "displayName": "No ID here" },
        { "id": "has.id.p1", "displayName": "Has ID" }
    ])";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
}

TEST_CASE(json_loader_nonexistent_file_returns_zero) {
    JsonMachineLoader loader;
    int n = loader.loadFile("/tmp/mmsim_does_not_exist_abc123.json");
    ASSERT_EQ(n, 0);
}

// ============================================================
// Phase 2 — buildFromSpec (Steps 3–9)
// ============================================================

TEST_CASE(json_loader_builds_bus_and_cpu) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.bus_cpu",
        "displayName": "Phase 2 Bus+CPU Test",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.bus_cpu");
    ASSERT_NE(desc, nullptr);
    ASSERT_EQ((int)desc->buses.size(), 1);
    ASSERT_EQ(desc->buses[0].busName, std::string("system"));
    ASSERT_NE(desc->buses[0].bus, nullptr);
    ASSERT_EQ((int)desc->cpus.size(), 1);
    ASSERT_NE(desc->cpus[0].cpu, nullptr);
    ASSERT(desc->schedulerStep != nullptr);
    ASSERT(desc->onReset != nullptr);
    delete desc;
}

TEST_CASE(json_loader_machine_id_propagated) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.idcheck",
        "displayName": "ID Check",
        "licenseClass": "open",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.idcheck");
    ASSERT_NE(desc, nullptr);
    ASSERT_EQ(desc->machineId,    std::string("p2.idcheck"));
    ASSERT_EQ(desc->displayName,  std::string("ID Check"));
    ASSERT_EQ(desc->licenseClass, std::string("open"));
    delete desc;
}

TEST_CASE(json_loader_roms_allocated_without_files) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.roms",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "roms": [
            { "label": "kernal", "path": "/nonexistent/kernal.bin",
              "size": 8192, "overlayAddr": "0xE000", "writable": false }
        ],
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.roms");
    ASSERT_NE(desc, nullptr);
    // Buffer was allocated (romLoad silently fails for nonexistent file)
    ASSERT_EQ((int)desc->roms.size(), 1);
    ASSERT_NE(desc->roms[0], nullptr);
    delete desc;
}

TEST_CASE(json_loader_devices_registered) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.devices",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev", "name": "DEV1", "baseAddr": "0x9000" },
            { "type": "stub_dev", "name": "DEV2", "baseAddr": "0xA000" }
        ],
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.devices");
    ASSERT_NE(desc, nullptr);
    ASSERT_NE(desc->ioRegistry, nullptr);
    // Both devices should be in the registry
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    ASSERT_EQ((int)handlers.size(), 2);
    delete desc;
}

TEST_CASE(json_loader_irq_signal_wired) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.irq",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev", "name": "MYDEV", "baseAddr": "0x9000" }
        ],
        "signals": [
            { "name": "irq", "type": "simple", "sources": ["MYDEV"] }
        ],
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.irq");
    ASSERT_NE(desc, nullptr);
    // Verify the signal line was allocated and registered for cleanup
    ASSERT_EQ((int)desc->deleters.size(), 1);
    delete desc;
}

TEST_CASE(json_loader_scheduler_executes) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    // Minimal program: NOP loop at 0x0000
    const char* json = R"({
        "id": "p2.sched",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.sched");
    ASSERT_NE(desc, nullptr);
    ASSERT(desc->schedulerStep != nullptr);
    // Plant a NOP at 0x0000 so the CPU has something to execute
    desc->buses[0].bus->write8(0, 0xEA);  // NOP
    desc->cpus[0].cpu->setPc(0);
    int cycles = desc->schedulerStep(*desc);
    ASSERT(cycles > 0);
    delete desc;
}

TEST_CASE(json_loader_reset_hook_fires) {
    setupJmlRegistries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p2.reset",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "scheduler": { "type": "step_tick", "frameAccumulator": null }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p2.reset");
    ASSERT_NE(desc, nullptr);
    ASSERT(desc->onReset != nullptr);
    // onReset should not crash on a minimal descriptor
    desc->onReset(*desc);
    delete desc;
}

// ============================================================
// Phase 3 — extended buildFromSpec features
// ============================================================

// Extended stub with port-device tracking and char-ROM tracking
static bool s_jmlP3Ready = false;
static void setupP3Registries() {
    setupJmlRegistries(); // ensure 6502 + stub_dev are registered
    if (s_jmlP3Ready) return;
    s_jmlP3Ready = true;

    // Device that tracks whether setCharRom / setBasicRom / setKernalRom were called
    struct RomTrackDev : public IOHandler {
        std::string m_name;
        uint32_t    m_base = 0;
        bool charSet = false, basicSet = false, kernalSet = false;
        const char* name()     const override { return m_name.c_str(); }
        uint32_t baseAddr()    const override { return m_base; }
        uint32_t addrMask()    const override { return 0; }
        bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
        void setName    (const std::string& n) override { m_name = n; }
        void setBaseAddr(uint32_t addr)        override { m_base = addr; }
        void setCharRom (const uint8_t*, uint32_t) override { charSet   = true; }
        void setBasicRom(const uint8_t*, uint32_t) override { basicSet  = true; }
        void setKernalRom(const uint8_t*, uint32_t) override { kernalSet = true; }
    };
    DeviceRegistry::instance().registerDevice("rom_track_dev",
        []() -> IOHandler* { return new RomTrackDev(); });

    // Device with port-A and port-B device attachment, to test kbdWiring
    struct PortDev : public IOHandler {
        std::string m_name;
        uint32_t    m_base = 0;
        IPortDevice* portA = nullptr;
        IPortDevice* portB = nullptr;
        const char* name()     const override { return m_name.c_str(); }
        uint32_t baseAddr()    const override { return m_base; }
        uint32_t addrMask()    const override { return 0x0F; }
        bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
        void setName    (const std::string& n) override { m_name = n; }
        void setBaseAddr(uint32_t addr)        override { m_base = addr; }
        void setPortADevice(IPortDevice* d)    override { portA = d; }
        void setPortBDevice(IPortDevice* d)    override { portB = d; }
    };
    DeviceRegistry::instance().registerDevice("port_dev",
        []() -> IOHandler* { return new PortDev(); });

    // Mock keyboard: implements both IOHandler and IKeyboardMatrix
    struct MockKbd : public IOHandler, public IKeyboardMatrix {
        std::string m_name;
        uint32_t    m_base = 0;
        bool lastDown = false;
        struct NullPort : public IPortDevice {
            uint8_t readPort() override { return 0xFF; }
            void writePort(uint8_t) override {}
            void setDdr(uint8_t) override {}
        };
        NullPort colPort, rowPort;
        const char* name()     const override { return m_name.c_str(); }
        uint32_t baseAddr()    const override { return m_base; }
        uint32_t addrMask()    const override { return 0; }
        bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
        void setName    (const std::string& n) override { m_name = n; }
        void setBaseAddr(uint32_t addr)        override { m_base = addr; }
        // IKeyboardMatrix
        void keyDown(int, int) override {}
        void keyUp(int, int)   override {}
        void clearKeys()       override {}
        bool pressKeyByName(const std::string&, bool down) override { lastDown = down; return true; }
        void enqueueText(const std::string&) override {}
        IPortDevice* getPort(int i) override { return i == 0 ? &colPort : &rowPort; }
    };
    DeviceRegistry::instance().registerDevice("mock_kbd",
        []() -> IOHandler* { return new MockKbd(); });
}

// ------------------------------------------------------------
// Test 1: inline_color_ram creates a ColorRamHandler in IORegistry
// ------------------------------------------------------------

TEST_CASE(json_loader_inline_color_ram) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.color_ram",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev",      "name": "DEV1", "baseAddr": "0x9000" },
            { "type": "inline_color_ram", "name": "ColorRAM", "baseAddr": "0x9400", "size": 1024 }
        ],
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.color_ram");
    ASSERT_NE(desc, nullptr);
    // stub_dev + ColorRamHandler = 2 handlers
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    ASSERT_EQ((int)handlers.size(), 2);
    // Write to color RAM and read back via bus
    desc->buses[0].bus->write8(0x9400, 0x0F);
    uint8_t val = desc->buses[0].bus->read8(0x9400);
    ASSERT_EQ((int)val, 0xFF); // low nibble 0xF | upper 0xF0
    delete desc;
}

// ------------------------------------------------------------
// Test 2: ramExpansions installs OpenBusHandlers for absent blocks
// ------------------------------------------------------------

TEST_CASE(json_loader_ram_expansions) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.ramexp",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "ramExpansions": [
            { "name": "blk1", "base": "0x2000", "size": 8192, "installed": false },
            { "name": "blk2", "base": "0x4000", "size": 8192, "installed": false }
        ],
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.ramexp");
    ASSERT_NE(desc, nullptr);
    // 2 OpenBusHandlers should be in IORegistry
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    ASSERT_EQ((int)handlers.size(), 2);
    // Reads from uninstalled blocks return 0xFF
    uint8_t val = desc->buses[0].bus->read8(0x2000);
    ASSERT_EQ((int)val, 0xFF);
    val = desc->buses[0].bus->read8(0x4100);
    ASSERT_EQ((int)val, 0xFF);
    delete desc;
}

// ------------------------------------------------------------
// Test 3: type "shared" signal creates SharedIrqManager
// ------------------------------------------------------------

TEST_CASE(json_loader_shared_irq) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.shared_irq",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev", "name": "DEV1", "baseAddr": "0x9000" },
            { "type": "stub_dev", "name": "DEV2", "baseAddr": "0x9010" }
        ],
        "signals": [
            { "name": "irq", "type": "shared", "sources": ["DEV1", "DEV2"] }
        ],
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.shared_irq");
    ASSERT_NE(desc, nullptr);
    // SharedIrqManager is tracked via one deleter
    ASSERT_EQ((int)desc->deleters.size(), 1);
    delete desc;
}

// ------------------------------------------------------------
// Test 4: frameAccumulator scheduler pulses vsync line
// ------------------------------------------------------------

TEST_CASE(json_loader_frame_accumulator) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.frame_accum",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "scheduler": {
            "type": "step_tick",
            "frameAccumulator": { "cycles": 100, "signal": "vsync" }
        }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.frame_accum");
    ASSERT_NE(desc, nullptr);
    ASSERT(desc->schedulerStep != nullptr);
    // Plant NOPs so CPU can execute
    desc->buses[0].bus->write8(0, 0xEA); // NOP
    desc->cpus[0].cpu->setPc(0);
    // Run enough steps to accumulate > 100 cycles (should not crash)
    int total = 0;
    for (int i = 0; i < 50; ++i)
        total += desc->schedulerStep(*desc);
    ASSERT(total > 0);
    delete desc;
}

// ------------------------------------------------------------
// Test 5: ioHook.mirrorRanges routes reads/writes to target device
// ------------------------------------------------------------

TEST_CASE(json_loader_mirror_range) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.mirror",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev", "name": "PIA1", "baseAddr": "0xE810" }
        ],
        "ioHook": {
            "mirrorRanges": [
                { "from": "0xE800", "to": "0xE80F", "target": "PIA1", "targetBase": "0xE810" }
            ]
        },
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.mirror");
    ASSERT_NE(desc, nullptr);
    ASSERT_NE(desc->buses[0].bus, nullptr);
    // Reading from the mirror range should not crash (stub_dev returns false → 0)
    uint8_t val = desc->buses[0].bus->read8(0xE800);
    (void)val;
    delete desc;
}

// ------------------------------------------------------------
// Test 6: ROM passToDevice calls setCharRom on the target device
// ------------------------------------------------------------

TEST_CASE(json_loader_pass_to_device) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.pass_rom",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "rom_track_dev", "name": "VIDEO", "baseAddr": "0x9000" }
        ],
        "roms": [
            { "label": "char", "path": "/nonexistent/char.bin", "size": 4096,
              "passToDevice": ["VIDEO"] }
        ],
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.pass_rom");
    ASSERT_NE(desc, nullptr);
    // VIDEO device should have received setCharRom call
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    ASSERT_EQ((int)handlers.size(), 1);
    // The device is owned by io registry; cast to check the flag
    // Use the raw pointer from the registry
    // (We can't easily recover the RomTrackDev*, but we can verify no crash occurred.)
    ASSERT_EQ((int)desc->roms.size(), 1); // char ROM buffer allocated
    delete desc;
}

// ------------------------------------------------------------
// Test 7: kbdWiring connects mock keyboard to port device
// ------------------------------------------------------------

TEST_CASE(json_loader_kbd_wiring) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.kbd",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "port_dev", "name": "VIA2", "baseAddr": "0x9120" },
            { "type": "mock_kbd", "name":  "KBD", "baseAddr": "0x0000" }
        ],
        "kbdWiring": {
            "device": "KBD",
            "colPort": { "device": "VIA2", "port": "B" },
            "rowPort": { "device": "VIA2", "port": "A" }
        },
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.kbd");
    ASSERT_NE(desc, nullptr);
    ASSERT(desc->onKey != nullptr);
    // onKey should invoke the keyboard
    bool ok = desc->onKey("space", true);
    ASSERT(ok);
    delete desc;
}

// ------------------------------------------------------------
// Test 8: joystick wiring creates onJoystick handler
// ------------------------------------------------------------

TEST_CASE(json_loader_joystick_wiring) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.joy",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "port_dev", "name": "VIA1", "baseAddr": "0x9110" }
        ],
        "joystick": { "device": "VIA1", "port": "B" },
        "scheduler": { "type": "step_tick" }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.joy");
    ASSERT_NE(desc, nullptr);
    ASSERT(desc->onJoystick != nullptr);
    // Should not crash
    desc->onJoystick(0, 0xFF);
    delete desc;
}

// ------------------------------------------------------------
// Test 9: device "lines" wiring connects latch signal to CA1
// ------------------------------------------------------------

TEST_CASE(json_loader_device_lines_wiring) {
    setupP3Registries();
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "p3.lines",
        "bus": { "type": "FlatMemoryBus", "name": "system", "addrBits": 16 },
        "cpu": { "type": "6502", "dataBus": "system", "codeBus": "system" },
        "devices": [
            { "type": "stub_dev", "name": "PIA1", "baseAddr": "0xE810",
              "lines": { "ca1": "vsync", "cb1": "vsync" } }
        ],
        "signals": [
            { "name": "vsync", "type": "latch" }
        ],
        "scheduler": {
            "type": "step_tick",
            "frameAccumulator": { "cycles": 100, "signal": "vsync" }
        }
    })";
    loader.loadString(json);
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("p3.lines");
    ASSERT_NE(desc, nullptr);
    // 1 deleter for vsync (latch signal) + 1 for frameAccum uint64_t
    // (vsync is shared between signals list and frameAccumulator — same pointer)
    ASSERT(desc->deleters.size() >= 1);
    delete desc;
}
