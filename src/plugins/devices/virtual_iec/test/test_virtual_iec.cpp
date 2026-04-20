#include "test_harness.h"
#include "virtual_iec.h"
#include <fstream>
#include <cstdio>

TEST_CASE(virtual_iec_disk_status) {
    VirtualIECBus iec(8);
    int t, s;
    bool led;

    // Initial status
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(t == 0);
    ASSERT(s == 0);
    ASSERT(led == false);

    // Mount a dummy file
    const char* dummyPath = "test_dummy.prg";
    std::ofstream f(dummyPath, std::ios::binary);
    f << "dummy data";
    f.close();

    ASSERT(iec.mountDisk(8, dummyPath));
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(t == 18);
    ASSERT(led == false);

    // Busy during tick
    iec.writePort(0x08); // ATN
    iec.tick(100);
    iec.tick(3000);
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(led == true);

    // Idle after reset
    iec.reset();
    ASSERT(iec.getDiskStatus(8, t, s, led));
    ASSERT(led == false);

    std::remove(dummyPath);
}

TEST_CASE(virtual_iec_g64_mount) {
    VirtualIECBus iec(8);
    const char* g64Path = "test.g64";
    std::ofstream f(g64Path, std::ios::binary);
    f << "GCR-1541";
    f.write("\x00\x00\x00\x00", 4);
    f.close();
    ASSERT(iec.mountDisk(8, g64Path));
    iec.writePort(0x08); // ATN
    iec.tick(3000);
    std::remove(g64Path);
}
