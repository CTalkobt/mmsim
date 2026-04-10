#include "test_harness.h"
#include "plugins/cbm-loader/main/d64_parser.h"
#include "plugins/cbm-loader/main/g64_parser.h"
#include "plugins/cbm-loader/main/t64_parser.h"
#include "plugins/cbm-loader/main/disk_loader.h"
#include "libmem/main/memory_bus.h"
#include <fstream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

TEST_CASE(d64_parsing) {
    std::string path = "test.d64";
    {
        std::ofstream file(path, std::ios::binary);
        std::vector<uint8_t> data(174848, 0); // 35 tracks
        
        // Track 18, Sector 0: BAM (minimal)
        uint32_t bamOff = (17 * 21) * 256;
        data[bamOff] = 18;
        data[bamOff + 1] = 1;
        
        // Track 18, Sector 1: Directory
        uint32_t dirOff = (17 * 21 + 1) * 256;
        data[dirOff] = 0; // End of chain
        data[dirOff + 1] = 0xFF;
        
        // Entry 1
        data[dirOff + 2] = 0x82; // PRG
        data[dirOff + 3] = 1;    // Track 1
        data[dirOff + 4] = 0;    // Sector 0
        std::memcpy(&data[dirOff + 5], "TESTFILE", 8);
        for(int i=13; i<21; ++i) data[dirOff+i] = 0xA0; // Padding
        data[dirOff + 30] = 1;   // 1 block
        
        // Track 1, Sector 0: File data
        uint32_t fileOff = 0;
        data[fileOff] = 0; // End of chain
        data[fileOff + 1] = 5; // 4 bytes of data (index 2,3,4,5)
        data[fileOff + 2] = 0x00; // Load addr lo
        data[fileOff + 3] = 0x10; // Load addr hi
        data[fileOff + 4] = 0xEA; // NOP
        data[fileOff + 5] = 0x60; // RTS
        
        file.write((char*)data.data(), data.size());
    }
    
    D64Parser parser;
    ASSERT(parser.open(path));
    
    auto dir = parser.getDirectory();
    ASSERT(dir.size() >= 1);
    ASSERT(dir[0].filename == "TESTFILE");
    ASSERT(dir[0].type == 0x82);
    
    std::vector<uint8_t> fileData;
    ASSERT(parser.readFile("TESTFILE", fileData));
    ASSERT(fileData.size() == 4);
    ASSERT(fileData[0] == 0x00);
    ASSERT(fileData[1] == 0x10);
    ASSERT(fileData[2] == 0xEA);
    ASSERT(fileData[3] == 0x60);
    
    fs::remove(path);
}

TEST_CASE(t64_parsing) {
    std::string path = "test.t64";
    {
        std::ofstream file(path, std::ios::binary);
        std::vector<uint8_t> data(1024, 0);
        
        std::memcpy(data.data(), "C64 tape image file", 20);
        data[34] = 1; // 1 entry
        data[35] = 0;
        
        // Entry 1 at offset 64
        data[64] = 1;    // Normal file
        data[65] = 0x82; // PRG
        data[66] = 0x00; // Start addr lo
        data[67] = 0x10; // Start addr hi
        data[68] = 0x04; // End addr lo
        data[69] = 0x10; // End addr hi ($1004)
        data[72] = 0x80; // Offset in file lo ($80 = 128)
        data[73] = 0x00;
        std::memcpy(&data[76], "T64FILE", 7);
        
        // File data at 128
        data[128] = 0xEA;
        data[129] = 0xEA;
        data[130] = 0xEA;
        data[131] = 0x60;
        
        file.write((char*)data.data(), data.size());
    }
    
    T64Parser parser;
    ASSERT(parser.open(path));
    
    auto dir = parser.getDirectory();
    ASSERT(dir.size() == 1);
    ASSERT(dir[0].filename == "T64FILE");
    
    std::vector<uint8_t> fileData;
    ASSERT(parser.readFile("T64FILE", fileData));
    ASSERT(fileData.size() == 4);
    ASSERT(fileData[0] == 0xEA);
    
    fs::remove(path);
}

TEST_CASE(g64_parsing) {
    std::string path = "test.g64";
    {
        std::ofstream file(path, std::ios::binary);
        std::vector<uint8_t> data(1024, 0);
        
        std::memcpy(data.data(), "GCR-1541", 8);
        data[9] = 1; // 1 track
        
        // Track 1 offset at 12
        data[12] = 0x20; // offset 32
        data[13] = 0x00;
        
        // Speed map at 12 + 1*4 = 16
        data[16] = 3; // speed 3
        
        // Track data at 32
        data[32] = 4; // 4 bytes
        data[33] = 0;
        data[34] = 0xAA;
        data[35] = 0xBB;
        data[36] = 0xCC;
        data[37] = 0xDD;
        
        file.write((char*)data.data(), data.size());
    }
    
    G64Parser parser;
    ASSERT(parser.open(path));
    
    std::vector<uint8_t> trackData;
    ASSERT(parser.getTrackData(1, trackData));
    ASSERT(trackData.size() == 4);
    ASSERT(trackData[0] == 0xAA);
    
    fs::remove(path);
}

TEST_CASE(disk_image_loader) {
    std::string path = "test_loader.d64";
    {
        std::ofstream file(path, std::ios::binary);
        std::vector<uint8_t> data(174848, 0);
        uint32_t bamOff = (17 * 21) * 256;
        data[bamOff] = 18;
        data[bamOff + 1] = 1;
        uint32_t dirOff = (17 * 21 + 1) * 256;
        data[dirOff] = 0;
        data[dirOff + 1] = 0xFF;
        data[dirOff + 2] = 0x82; // PRG
        data[dirOff + 3] = 1;
        data[dirOff + 4] = 0;
        std::memcpy(&data[dirOff + 5], "LOADER  ", 8);
        for(int i=13; i<21; ++i) data[dirOff+i] = 0xA0;
        
        uint32_t fileOff = 0;
        data[fileOff] = 0;
        data[fileOff + 1] = 5;
        data[fileOff + 2] = 0x00;
        data[fileOff + 3] = 0x20; // $2000
        data[fileOff + 4] = 0x60; // RTS
        
        file.write((char*)data.data(), data.size());
    }
    
    FlatMemoryBus bus("test", 16);
    DiskImageLoader loader;
    ASSERT(loader.canLoad(path));
    ASSERT(loader.load(path, &bus, nullptr));
    
    ASSERT(bus.read8(0x2000) == 0x60);
    
    fs::remove(path);
}
