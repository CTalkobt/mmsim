#include "test_harness.h"
#include "libcore/main/json_machine_loader.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machine_desc.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_handler.h"
#include "libmem/main/memory_bus.h"
#include "cpu6502.h"

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
