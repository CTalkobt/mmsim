#include "test_harness.h"
#include "cli/main/cli_interpreter.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdebug/main/debug_context.h"
#include "cpu6502.h"
#include "via6522.h"
#include "vic6560.h"
#include "kbd_vic20.h"
#include <vector>
#include <string>

static bool s_bpRegistriesReady = false;
static void setupBreakpointTestRegistries() {
    if (s_bpRegistriesReady) return;
    s_bpRegistriesReady = true;
    CoreRegistry::instance().registerCore("6502", "NMOS", "open",
        []() -> ICore* { return new MOS6502(); });
    DeviceRegistry::instance().registerDevice("6560",
        []() -> IOHandler* { return new VIC6560("VIC-I", 0x9000); });
    DeviceRegistry::instance().registerDevice("6522",
        []() -> IOHandler* { return new VIA6522("6522", 0); });
    // KbdVic20 is IKeyboardMatrix only; wrap in a minimal IOHandler for the registry
    struct KbdWrapper : public IOHandler {
        KbdWrapper() : m_kbd(new KbdVic20()) {}
        ~KbdWrapper() { delete m_kbd; }
        const char* name() const override { return "kbd_vic20"; }
        uint32_t baseAddr() const override { return 0; }
        uint32_t addrMask() const override { return 0; }
        bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t) override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
        KbdVic20* m_kbd;
    };
    DeviceRegistry::instance().registerDevice("kbd_vic20",
        []() -> IOHandler* { return new KbdWrapper(); });
    JsonMachineLoader().loadFile("machines/vic20.json");
}

TEST_CASE(TestBasicExecBreakpoint) {
    setupBreakpointTestRegistries();
    CliContext ctx;
    std::vector<std::string> output;
    auto outputFn = [&](const std::string& s) { output.push_back(s); };
    CliInterpreter cli(ctx, outputFn);

    cli.processLine("create vic20");
    ASSERT_NE(ctx.machine, nullptr);
    ASSERT_NE(ctx.cpu, nullptr);
    ASSERT_NE(ctx.bus, nullptr);
    ASSERT_NE(ctx.dbg, nullptr);

    // Assemble a simple program
    cli.processLine("asm 0400");
    cli.processLine("JMP $0400");
    cli.processLine(".");

    // Set a breakpoint
    cli.processLine("break 0400");

    // Run the machine
    cli.processLine("run 0400");

    EXPECT_TRUE(ctx.dbg->isPaused());
    EXPECT_EQ(ctx.cpu->pc(), 0x0400);
}
