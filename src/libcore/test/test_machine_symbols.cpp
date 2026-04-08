#include "test_harness.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/machine_desc.h"
#include "libdebug/main/debug_context.h"
#include "cli/main/cli_interpreter.h"
#include "plugin_loader/main/plugin_loader.h"

TEST_CASE(machine_symbol_autoload) {
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    
    // Test C64
    cli.processLine("create c64");
    ASSERT_NE(ctx.dbg, nullptr);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CHROUT"), 0xFFD2);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("SETNAM"), 0xFFBD);
    
    // Test VIC-20
    cli.processLine("create vic20");
    ASSERT_NE(ctx.dbg, nullptr);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CHROUT"), 0xFFD2);
    // VIC-20 specific reset
    EXPECT_EQ(ctx.dbg->symbols().getAddress("RESET"), 0xE5CF);
    
    // Test PET
    cli.processLine("create pet4032");
    ASSERT_NE(ctx.dbg, nullptr);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CHROUT"), 0xFFD2);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("RESET"), 0xFD17);

    // Cleanup via PluginLoader
}
