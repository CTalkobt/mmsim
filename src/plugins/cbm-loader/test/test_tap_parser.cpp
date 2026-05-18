#include "test_harness.h"
#include "plugins/cbm-loader/main/tap_parser.h"
#include <filesystem>
#include <fstream>
#include <cstring>

namespace fs = std::filesystem;

static fs::path writeTapFile(uint8_t version, const std::vector<uint8_t>& data) {
    fs::path p = fs::temp_directory_path() / "mmemu_test.tap";
    std::ofstream f(p, std::ios::binary);

    // Header: 12-byte signature
    f.write("C64-TAPE-RAW", 12);
    // Version
    f.write((const char*)&version, 1);
    // 3 reserved bytes
    uint8_t reserved[3] = {0, 0, 0};
    f.write((const char*)reserved, 3);
    // Data size (LE 32-bit)
    uint32_t sz = (uint32_t)data.size();
    uint8_t sizeBytes[4] = {
        (uint8_t)(sz & 0xFF), (uint8_t)((sz >> 8) & 0xFF),
        (uint8_t)((sz >> 16) & 0xFF), (uint8_t)((sz >> 24) & 0xFF)
    };
    f.write((const char*)sizeBytes, 4);
    // Data
    if (!data.empty())
        f.write((const char*)data.data(), data.size());

    return p;
}

TEST_CASE(tap_load_valid) {
    // Simple TAP with a few normal pulses
    std::vector<uint8_t> pulseData = {0x30, 0x40, 0x50};
    auto path = writeTapFile(1, pulseData);

    TapArchive tap;
    ASSERT(tap.load(path.string()));
    ASSERT_EQ(tap.version(), (uint8_t)1);
    ASSERT_EQ(tap.dataSize(), (uint32_t)3);
    ASSERT_EQ(tap.data().size(), (size_t)3);

    fs::remove(path);
}

TEST_CASE(tap_load_nonexistent) {
    TapArchive tap;
    ASSERT(!tap.load("/tmp/mmemu_nonexistent.tap"));
}

TEST_CASE(tap_load_bad_signature) {
    fs::path p = fs::temp_directory_path() / "mmemu_test_badsig.tap";
    std::ofstream f(p, std::ios::binary);
    f.write("NOT-A-TAP!!!", 12);
    uint8_t rest[8] = {};
    f.write((const char*)rest, 8);
    f.close();

    TapArchive tap;
    ASSERT(!tap.load(p.string()));
    fs::remove(p);
}

TEST_CASE(tap_next_pulse_normal) {
    // Normal byte: value * 8
    std::vector<uint8_t> pulseData = {0x10, 0x20, 0xFF};
    auto path = writeTapFile(1, pulseData);

    TapArchive tap;
    ASSERT(tap.load(path.string()));

    uint32_t offset = 0;
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)(0x10 * 8));
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)(0x20 * 8));
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)(0xFF * 8));
    // End of data
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)0);

    fs::remove(path);
}

TEST_CASE(tap_next_pulse_long) {
    // Long pulse: 0x00 followed by 3 LE bytes
    std::vector<uint8_t> pulseData = {0x00, 0x40, 0x01, 0x00};
    auto path = writeTapFile(1, pulseData);

    TapArchive tap;
    ASSERT(tap.load(path.string()));

    uint32_t offset = 0;
    // 0x00 + 3 bytes = 0x000140 = 320
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)0x0140);

    fs::remove(path);
}

TEST_CASE(tap_next_pulse_truncated_long) {
    // Long pulse with insufficient trailing bytes
    std::vector<uint8_t> pulseData = {0x00, 0x40};
    auto path = writeTapFile(1, pulseData);

    TapArchive tap;
    ASSERT(tap.load(path.string()));

    uint32_t offset = 0;
    // Should return 0 (incomplete long pulse)
    ASSERT_EQ(tap.nextPulse(offset), (uint32_t)0);

    fs::remove(path);
}

TEST_CASE(tap_recording_roundtrip) {
    TapArchive tap;
    ASSERT(!tap.isRecording());

    tap.startRecording();
    ASSERT(tap.isRecording());

    // Normal pulse (divisible by 8, fits in one byte)
    tap.appendPulse(0x80);   // 128 / 8 = 16 = 0x10
    // Long pulse (zero cycles = triggers long encoding)
    tap.appendPulse(0);
    // Another normal
    tap.appendPulse(0x400);  // 1024 / 8 = 128

    tap.stopRecording();
    ASSERT(!tap.isRecording());

    fs::path p = fs::temp_directory_path() / "mmemu_test_record.tap";
    ASSERT(tap.saveRecording(p.string()));

    // Reload and verify
    TapArchive tap2;
    ASSERT(tap2.load(p.string()));
    ASSERT_EQ(tap2.version(), (uint8_t)1);
    ASSERT(tap2.dataSize() > 0);

    fs::remove(p);
}

TEST_CASE(tap_pulse_count) {
    TapArchive tap;
    tap.startRecording();
    tap.appendPulse(0x80);
    tap.appendPulse(0x100);
    ASSERT_EQ(tap.recordedPulseCount(), (uint32_t)2);
    tap.stopRecording();
}
