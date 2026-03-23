#include "test_harness.h"
#include "libcore/rom_loader.h"

TEST_CASE(smoke_test) {
    ASSERT(1 == 1);
}

TEST_CASE(rom_loader_placeholder) {
    // Just verify the symbol links
    uint8_t buf[1];
    (void)buf;
    // We don't have a real file to load yet, but we verify it doesn't crash on null
    bool result = romLoad("nonexistent_file_test", buf, 1);
    ASSERT(result == false);
}

// No main here
