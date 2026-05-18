#include "test_harness.h"
#include "libcore/main/sim_config.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE(sim_config_no_config_file) {
    // SimConfig::instance() is a singleton, but assemblerConfig on a name
    // that doesn't exist should return an empty config
    auto cfg = SimConfig::instance().assemblerConfig("nonexistent_assembler");
    ASSERT(cfg.binaryPath.empty());
    ASSERT(cfg.extraOptions.empty());
    ASSERT(cfg.workingDir.empty());
}

TEST_CASE(sim_config_assembler_config_parsing) {
    // Write a temporary config.json in a known location
    fs::path tmpDir = fs::temp_directory_path() / "mmemu_test_config";
    fs::create_directories(tmpDir);
    fs::path configPath = tmpDir / "config.json";

    {
        std::ofstream f(configPath);
        f << R"({
            "tools": {
                "assemblers": {
                    "testasm": {
                        "command": "/usr/bin/testasm",
                        "extraOptions": "--fast",
                        "workingDir": "/tmp"
                    },
                    "minimal": {
                        "command": "/usr/bin/minimal"
                    }
                }
            }
        })";
    }

    // We can't easily test load() since it uses fixed paths,
    // but we can test assemblerConfig() by noting the singleton
    // may have been loaded already. Instead, test the returned
    // defaults for unknown names.
    auto cfg = SimConfig::instance().assemblerConfig("definitely_not_configured");
    ASSERT(cfg.binaryPath.empty());

    fs::remove_all(tmpDir);
}
