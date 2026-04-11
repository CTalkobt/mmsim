#include "test_harness.h"
#include "cli/main/plugin_command_registry.h"
#include "mcp/main/plugin_tool_registry.h"
#include "gui/main/plugin_pane_manager.h"
#include "libdevices/main/ivideo_output.h"
#include <cstring>

static int testCmdExecute(int argc, const char* const* argv, void* ctx) {
    (void)argc; (void)argv;
    int* callCount = (int*)ctx;
    (*callCount)++;
    return 0;
}

TEST_CASE(plugin_extension_cli) {
    int callCount = 0;
    PluginCommandInfo info;
    info.name = "testcmd";
    info.usage = "test usage";
    info.execute = testCmdExecute;
    info.ctx = &callCount;

    ASSERT(PluginCommandRegistry::instance().registerCommand(&info));
    
    std::vector<std::string> tokens = {"testcmd", "arg1"};
    ASSERT(PluginCommandRegistry::instance().dispatch(tokens));
    ASSERT(callCount == 1);
    
    // Duplicate registration should fail
    ASSERT(!PluginCommandRegistry::instance().registerCommand(&info));
}

TEST_CASE(plugin_extension_cli_collision) {
    PluginCommandRegistry::instance().registerBuiltIn("help");
    
    PluginCommandInfo info;
    info.name = "help";
    info.usage = "fake help";
    info.execute = testCmdExecute;
    info.ctx = nullptr;

    ASSERT(!PluginCommandRegistry::instance().registerCommand(&info));
}

static void testToolHandle(const char* paramsJson, char** resultJson, void* ctx) {
    (void)paramsJson;
    int* callCount = (int*)ctx;
    (*callCount)++;
    *resultJson = strdup("{\"status\": \"ok\"}");
}

static void testToolFree(char* s) {
    free(s);
}

TEST_CASE(plugin_extension_mcp) {
    int callCount = 0;
    PluginMcpToolInfo info;
    info.toolName = "testtool";
    info.schemaJson = "{\"type\": \"object\"}";
    info.handle = testToolHandle;
    info.freeString = testToolFree;
    info.ctx = &callCount;

    ASSERT(PluginToolRegistry::instance().registerTool(&info));
    
    std::string result;
    ASSERT(PluginToolRegistry::instance().dispatch("testtool", "{}", result));
    ASSERT(callCount == 1);
    ASSERT(result == "{\"status\": \"ok\"}");
}

TEST_CASE(plugin_extension_gui_logic) {
    auto createFn = [](void* parent, void* ctx) -> void* {
        (void)parent;
        int* c = (int*)ctx;
        (*c)++;
        return (void*)0x1234; // Non-null dummy handle
    };
    auto refreshFn = [](void* handle, uint64_t cycles, void* ctx) {
        (void)handle; (void)cycles;
        int* c = (int*)((uint8_t*)ctx + sizeof(int));
        (*c)++;
    };
    auto destroyFn = [](void* handle, void* ctx) {
        (void)handle;
        int* c = (int*)((uint8_t*)ctx + 2 * sizeof(int));
        (*c)++;
    };

    struct TestCtx {
        int create;
        int refresh;
        int destroy;
    } ctx = {0, 0, 0};

    const char* machines[] = {"vic20", nullptr};
    PluginPaneInfo info;
    std::memset(&info, 0, sizeof(info));
    info.paneId = "vic-only-pane";
    info.displayName = "VIC-20 Tool";
    info.menuSection = "Tools";
    info.machineIds = machines;
    info.createPane = createFn;
    info.refreshPane = refreshFn;
    info.destroyPane = destroyFn;
    info.onMachineLoad = nullptr;
    info.ctx = &ctx;

    PluginPaneManager::instance().registerPane(&info);
    
    wxWindow* dummyParent = (wxWindow*)0x5678;

    // Switch to non-matching machine
    PluginPaneManager::instance().onMachineSwitch("c64", dummyParent, nullptr);
    ASSERT(ctx.create == 0);
    
    // Switch to matching machine
    PluginPaneManager::instance().onMachineSwitch("vic20", dummyParent, nullptr);
    ASSERT(ctx.create == 1);
    
    // Tick
    PluginPaneManager::instance().tickAll(100);
    ASSERT(ctx.refresh == 1);
    
    // Switch away
    PluginPaneManager::instance().onMachineSwitch("c64", dummyParent, nullptr);
    ASSERT(ctx.destroy == 1);
}
