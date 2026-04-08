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
    EXPECT_EQ(ctx.dbg->symbols().getAddress("READY"),  0xA474);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("MAIN"),   0xA483);
    
    // Test VIC-20
    cli.processLine("create vic20");
    ASSERT_NE(ctx.dbg, nullptr);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CHROUT"), 0xFFD2);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("READY"),  0xC474);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CLR"),    0xC65E);
    
    // Test PET
    cli.processLine("create pet4032");
    ASSERT_NE(ctx.dbg, nullptr);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CHROUT"), 0xFFD2);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("READY"),  0xB3FF);
    EXPECT_EQ(ctx.dbg->symbols().getAddress("CRUNCH"), 0xB4FB);

    // Cleanup via PluginLoader
}
