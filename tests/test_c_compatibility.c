#include "include/mmemu_plugin_api.h"
#include <stddef.h>

/*
 * This file verifies that the plugin API header is valid C.
 * It does not need to be executed, only compiled.
 */

static void my_handle(const char* params, char** result, void* ctx) {
    (void)params; (void)result; (void)ctx;
}

static void my_free(char* s) {
    (void)s;
}

void test_structures(void) {
    struct PluginPaneInfo pane;
    pane.paneId = "test";
    pane.createPane = NULL;
    (void)pane;

    struct PluginCommandInfo cmd;
    cmd.name = "test";
    cmd.execute = NULL;
    (void)cmd;

    struct PluginMcpToolInfo tool;
    tool.toolName = "test";
    tool.handle = my_handle;
    tool.freeString = my_free;
    (void)tool;

    struct SimPluginHostAPI host;
    host.log = NULL;
    host.registerPane = NULL;
    (void)host;
}
