#include "test_harness.h"
#include "libcore/main/json_machine_loader.h"
#include "libcore/main/machines/machine_registry.h"

// ---------------------------------------------------------------------------
// Smoke tests for JsonMachineLoader — Phase 1 (skeleton)
// ---------------------------------------------------------------------------

TEST_CASE(json_loader_parse_valid_single) {
    JsonMachineLoader loader;
    const char* json = R"({
        "id": "test.machine",
        "displayName": "Test Machine"
    })";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
    // Factory is registered; buildFromSpec is a stub so createMachine returns nullptr.
    MachineDescriptor* desc = MachineRegistry::instance().createMachine("test.machine");
    ASSERT_EQ(desc, nullptr);
}

TEST_CASE(json_loader_parse_array) {
    JsonMachineLoader loader;
    const char* json = R"([
        { "id": "arr.machine.a", "displayName": "A" },
        { "id": "arr.machine.b", "displayName": "B" }
    ])";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 2);
}

TEST_CASE(json_loader_parse_machines_key) {
    JsonMachineLoader loader;
    const char* json = R"({
        "machines": [
            { "id": "wrapped.machine.x", "displayName": "X" }
        ]
    })";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
}

TEST_CASE(json_loader_invalid_json_returns_zero) {
    JsonMachineLoader loader;
    int n = loader.loadString("this is not json {{{{");
    ASSERT_EQ(n, 0);
}

TEST_CASE(json_loader_missing_id_skipped) {
    JsonMachineLoader loader;
    const char* json = R"([
        { "displayName": "No ID here" },
        { "id": "has.id", "displayName": "Has ID" }
    ])";
    int n = loader.loadString(json);
    ASSERT_EQ(n, 1);
}

TEST_CASE(json_loader_nonexistent_file_returns_zero) {
    JsonMachineLoader loader;
    int n = loader.loadFile("/tmp/mmsim_does_not_exist_abc123.json");
    ASSERT_EQ(n, 0);
}
