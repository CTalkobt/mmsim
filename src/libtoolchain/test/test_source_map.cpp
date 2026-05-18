#include "test_harness.h"
#include "libtoolchain/main/source_map.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

static fs::path writeTempList(const std::string& content) {
    fs::path p = fs::temp_directory_path() / "mmemu_test_kickass.lst";
    std::ofstream f(p);
    f << content;
    return p;
}

TEST_CASE(source_map_load_kickass_list) {
    // Simulate a KickAssembler listing:
    //   [1000] 4c 00 10  1  main: jmp main
    //   [1003] a9 42     2  lda #$42
    auto path = writeTempList(
        "[1000] 4c 00 10  1  main: jmp main\n"
        "[1003] a9 42     2  lda #$42\n"
        "  ; comment line\n"
        "\n"
        "[2000] 60        5  rts\n"
    );

    SourceMap sm;
    ASSERT(sm.loadKickAssList(path.string()));

    // Addresses should be parsed
    auto loc1 = sm.addrToSource(0x1000);
    ASSERT(!loc1.file.empty());

    auto loc2 = sm.addrToSource(0x1003);
    ASSERT(!loc2.file.empty());

    auto loc3 = sm.addrToSource(0x2000);
    ASSERT(!loc3.file.empty());

    // Unknown address
    auto locMissing = sm.addrToSource(0xFFFF);
    ASSERT(locMissing.file.empty());
    ASSERT_EQ(locMissing.line, -1);

    fs::remove(path);
}

TEST_CASE(source_map_load_nonexistent) {
    SourceMap sm;
    ASSERT(!sm.loadKickAssList("/tmp/mmemu_test_nonexistent_file.lst"));
}

TEST_CASE(source_map_source_to_addr) {
    SourceMap sm;
    // sourceToAddr on empty map returns 0xFFFFFFFF
    ASSERT_EQ(sm.sourceToAddr("test.asm", 1), (uint32_t)0xFFFFFFFF);
    ASSERT_EQ(sm.sourceToAddr("nonexistent.asm", 99), (uint32_t)0xFFFFFFFF);
}

TEST_CASE(source_map_empty_file) {
    auto path = writeTempList("");
    SourceMap sm;
    ASSERT(sm.loadKickAssList(path.string()));
    auto loc = sm.addrToSource(0x0000);
    ASSERT(loc.file.empty());
    fs::remove(path);
}

TEST_CASE(source_map_malformed_lines) {
    // Lines without brackets should be skipped
    auto path = writeTempList(
        "this is not a valid line\n"
        "[abcd incomplete line\n"
        "[CAFE] ea       nop\n"
    );
    SourceMap sm;
    ASSERT(sm.loadKickAssList(path.string()));
    auto loc = sm.addrToSource(0xCAFE);
    ASSERT(!loc.file.empty());
    fs::remove(path);
}
