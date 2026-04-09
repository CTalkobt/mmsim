#include "test_harness.h"
#include "include/mmemu_plugin_api.h"
#include "include/util/logging.h"

int main() {
    LogRegistry::instance().init();
    return runAllTests();
}
