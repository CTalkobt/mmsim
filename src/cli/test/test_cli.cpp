#include "test_harness.h"
#include "cli_interpreter.h"
#include "plugin_command_registry.h"
#include "plugins/6502/main/cpu6502.h"
#include "plugins/6502/main/assembler_6502.h"
#include "libmem/main/memory_bus.h"
#include "libdebug/main/debug_context.h"
#include <vector>
#include <string>
#include <algorithm>

TEST_CASE(cli_basic_commands) {
    CliContext ctx;
    std::string output;
    CliInterpreter interpreter(ctx, [&](const std::string& s) { output += s; });

    // help command
    interpreter.processLine("help");
    ASSERT(output.find("Available commands") != std::string::npos);
    output.clear();

    // unknown command
    interpreter.processLine("garbage_command");
    ASSERT(output.find("Unknown command") != std::string::npos);
}

TEST_CASE(cli_state_management) {
    CliContext ctx;
    FlatMemoryBus bus("system", 16);
    MOS6502 cpu;
    Assembler6502 assem;
    cpu.setDataBus(&bus);
    ctx.bus = &bus;
    ctx.cpu = &cpu;
    ctx.assem = &assem;

    std::string output;
    CliInterpreter interpreter(ctx, [&](const std::string& s) { output += s; });

    // Test quit
    ASSERT(ctx.quit == false);
    interpreter.processLine("quit");
    ASSERT(ctx.quit == true);

    // Test asm mode state
    ASSERT(interpreter.isAssemblyMode() == false);
    interpreter.processLine("asm $1000");
    ASSERT(interpreter.isAssemblyMode() == true);
    ASSERT(interpreter.getAsmAddr() == 0x1000);

    // Assembly line (NOP is $EA)
    interpreter.processLine("NOP");
    ASSERT(bus.read8(0x1000) == 0xEA);
    ASSERT(interpreter.getAsmAddr() == 0x1001);

    // Exit asm mode
    interpreter.processLine(".");
    ASSERT(interpreter.isAssemblyMode() == false);
}

TEST_CASE(cli_error_handling) {
    CliContext ctx;
    std::string output;
    CliInterpreter interpreter(ctx, [&](const std::string& s) { output += s; });

    // Try commands that require a machine without having one
    interpreter.processLine("step");
    ASSERT(output.find("No machine created") != std::string::npos);
    output.clear();

    interpreter.processLine("m 1000");
    ASSERT(output.find("No machine created") != std::string::npos);
    output.clear();

    interpreter.processLine("regs");
    ASSERT(output.find("No machine created") != std::string::npos);
}

// Plugin Registry Tests
static int test_cmd_called = 0;
static int test_cmd_execute(int argc, const char* const* argv, void* ctx) {
    test_cmd_called++;
    return 0;
}

TEST_CASE(plugin_registry_registration) {
    PluginCommandRegistry& reg = PluginCommandRegistry::instance();
    
    PluginCommandInfo info;
    info.name = "cli_test_cmd";
    info.usage = "test usage";
    info.execute = test_cmd_execute;
    info.ctx = nullptr;

    // Register
    bool ok = reg.registerCommand(&info);
    ASSERT(ok == true);

    // Duplicate registration should fail
    ok = reg.registerCommand(&info);
    ASSERT(ok == false);

    // Built-in collision
    reg.registerBuiltIn("builtincmd");
    info.name = "builtincmd";
    ok = reg.registerCommand(&info);
    ASSERT(ok == false);
}

TEST_CASE(plugin_registry_dispatch) {
    PluginCommandRegistry& reg = PluginCommandRegistry::instance();
    
    test_cmd_called = 0;
    std::vector<std::string> tokens = {"cli_test_cmd", "arg1"};
    
    bool handled = reg.dispatch(tokens);
    ASSERT(handled == true);
    ASSERT(test_cmd_called == 1);

    tokens = {"unknown_plugin_cmd"};
    handled = reg.dispatch(tokens);
    ASSERT(handled == false);
}
