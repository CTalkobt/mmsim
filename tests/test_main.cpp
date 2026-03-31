#include "test_harness.h"
#include "include/mmemu_plugin_api.h"

// Mock globals for plugins when linked into the test binary
const SimPluginHostAPI* g_petHost = nullptr;

int main() {
    return runAllTests();
}
