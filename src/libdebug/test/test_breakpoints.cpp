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
#include "plugin_loader/main/plugin_loader.h"
#include <vector>
#include <string>

static void setupBreakpointTestRegistries() {
    PluginLoader::instance().loadFromDir("./lib");
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
    cli.processLine("asm $0400");
    cli.processLine("JMP $0400");
    cli.processLine(".");

    // Set a breakpoint
    cli.processLine("break $0400");

    // Run the machine
    cli.processLine("run $0400");

    EXPECT_TRUE(ctx.dbg->isPaused());
    EXPECT_EQ(ctx.cpu->pc(), 0x0400);
    
}
