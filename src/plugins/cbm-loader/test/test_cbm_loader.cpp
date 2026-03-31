#include "test_harness.h"
#include "libcore/main/image_loader.h"
#include "libcore/main/machine_desc.h"
#include "libmem/main/memory_bus.h"
#include "plugins/cbm-loader/main/prg_loader.h"
#include "plugins/cbm-loader/main/crt_parser.h"
#include "plugins/cbm-loader/main/cbm_cart_handler.h"
#include <fstream>
#include <cstdio>
#include <cstring>

TEST_CASE(BinLoader_Smoke) {
    auto* bus = new FlatMemoryBus("system", 16);
    auto* machine = new MachineDescriptor();
    machine->buses.push_back({"system", bus});

    // Create a dummy bin file
    {
        std::ofstream f("test.bin", std::ios::binary);
        uint8_t data[] = { 0x11, 0x22, 0x33, 0x44 };
        f.write((char*)data, 4);
    }

    auto loader = ImageLoaderRegistry::instance().findLoader("test.bin");
    ASSERT(loader != nullptr);

    bool success = loader->load("test.bin", bus, machine, 0x1000);
    ASSERT(success);
    ASSERT(bus->read8(0x1000) == 0x11);
    ASSERT(bus->read8(0x1001) == 0x22);
    ASSERT(bus->read8(0x1002) == 0x33);
    ASSERT(bus->read8(0x1003) == 0x44);

    std::remove("test.bin");
    delete machine;
}

TEST_CASE(PrgLoader_Smoke) {
    PrgLoader loader;
    auto* bus = new FlatMemoryBus("system", 16);
    auto* machine = new MachineDescriptor();
    machine->buses.push_back({"system", bus});
    
    // Create a dummy prg file (load address $2000)
    {
        std::ofstream f("test.prg", std::ios::binary);
        uint8_t header[] = { 0x00, 0x20 };
        f.write((char*)header, 2);
        uint8_t data[] = { 0xAA, 0xBB, 0xCC };
        f.write((char*)data, 3);
    }

    ASSERT(loader.canLoad("test.prg"));
    bool success = loader.load("test.prg", bus, machine);
    ASSERT(success);
    ASSERT(bus->read8(0x2000) == 0xAA);
    ASSERT(bus->read8(0x2001) == 0xBB);
    ASSERT(bus->read8(0x2002) == 0xCC);

    std::remove("test.prg");
    delete machine;
}

TEST_CASE(CrtParser_Smoke) {
    // Create a dummy CRT file
    {
        std::ofstream f("test.crt", std::ios::binary);
        // Header
        f.write("C64 CARTRIDGE   ", 16);
        uint8_t headerLen[] = { 0x00, 0x00, 0x00, 0x40 };
        f.write((char*)headerLen, 4);
        uint8_t version[] = { 0x01, 0x00 };
        f.write((char*)version, 2);
        uint8_t cartType[] = { 0x00, 0x00 };
        f.write((char*)cartType, 2);
        f.put(0x01); // exrom
        f.put(0x01); // game
        for(int i=0; i<6; i++) f.put(0); // reserved
        char name[32] = {0};
        std::strncpy(name, "Test Cartridge", 31);
        f.write(name, 32);

        // Chip Packet
        f.write("CHIP", 4);
        uint8_t packetLen[] = { 0x00, 0x00, 0x00, 0x0A + 4 }; // 10 bytes + 4 sig
        f.write((char*)packetLen, 4);
        uint8_t chipType[] = { 0x00, 0x00 };
        f.write((char*)chipType, 2);
        uint8_t bankNum[] = { 0x00, 0x00 };
        f.write((char*)bankNum, 2);
        uint8_t loadAddr[] = { 0x80, 0x00 };
        f.write((char*)loadAddr, 2);
        uint8_t imageSize[] = { 0x00, 0x04 };
        f.write((char*)imageSize, 2);
        uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
        f.write((char*)data, 4);
    }

    CrtParser parser;
    bool success = parser.parse("test.crt");
    ASSERT(success);
    ASSERT(parser.header().cartType == 0);
    ASSERT(std::string(parser.header().cartName) == "Test Cartridge");
    ASSERT(parser.chips().size() == 1);
    ASSERT(parser.chips()[0].loadAddr == 0x8000);
    ASSERT(parser.chips()[0].data.size() == 4);
    ASSERT(parser.chips()[0].data[0] == 0xDE);

    std::remove("test.crt");
}

TEST_CASE(Registry_Dispatch) {
    auto& reg = ImageLoaderRegistry::instance();
    
    // BIN loader is built-in
    ASSERT(reg.findLoader("foo.bin") != nullptr);
    
    // PRG loader is in the plugin, but we've linked it into the test binary
    // via DEVICE_TEST_OBJS and it registers itself in plugin_init.cpp
    // (In this test environment, we might need to manually register it if 
    // plugin_init isn't called, but ImageLoaderRegistry::instance() 
    // registers the default ones).
    
    // Actually, in the test binary, we are linking the .o files directly.
    // Let's ensure PRG loader is registered for the test.
    reg.registerLoader(std::make_unique<PrgLoader>());
    ASSERT(reg.findLoader("foo.prg") != nullptr);
    ASSERT(reg.findLoader("foo.txt") == nullptr);
}

TEST_CASE(Cartridge_Attach_Eject) {
    // Create a dummy CRT file
    const char* cartPath = "test_attach.crt";
    {
        std::ofstream f(cartPath, std::ios::binary);
        f.write("C64 CARTRIDGE   ", 16);
        uint8_t headerLen[] = { 0x00, 0x00, 0x00, 0x40 };
        f.write((char*)headerLen, 4);
        uint8_t version[] = { 0x01, 0x00 };
        f.write((char*)version, 2);
        uint8_t cartType[] = { 0x00, 0x00 };
        f.write((char*)cartType, 2);
        f.put(0x01); // exrom
        f.put(0x01); // game
        for(int i=0; i<6; i++) f.put(0); // reserved
        char name[32] = {0};
        std::strncpy(name, "Test Attach", 31);
        f.write(name, 32);

        f.write("CHIP", 4);
        uint8_t packetLen[] = { 0x00, 0x00, 0x00, 0x0A + 4 };
        f.write((char*)packetLen, 4);
        uint8_t chipType[] = { 0x00, 0x00 };
        f.write((char*)chipType, 2);
        uint8_t bankNum[] = { 0x00, 0x00 };
        f.write((char*)bankNum, 2);
        uint8_t loadAddr[] = { 0x80, 0x00 };
        f.write((char*)loadAddr, 2);
        uint8_t imageSize[] = { 0x00, 0x04 };
        f.write((char*)imageSize, 2);
        uint8_t data[] = { 0x11, 0x22, 0x33, 0x44 };
        f.write((char*)data, 4);
    }

    FlatMemoryBus bus("system", 16);
    // Write something to RAM at $8000 first
    bus.write8(0x8000, 0x99);
    ASSERT(bus.read8(0x8000) == 0x99);

    auto handler = std::make_unique<CbmCartridgeHandler>(cartPath);
    bool attached = handler->attach(&bus, nullptr);
    ASSERT(attached);

    // Verify overlay is active
    ASSERT(bus.read8(0x8000) == 0x11);
    ASSERT(bus.read8(0x8001) == 0x22);

    // Verify metadata
    auto md = handler->metadata();
    ASSERT(md.displayName == "Test Attach");
    ASSERT(md.startAddr == 0x8000);

    // Eject
    handler->eject(&bus);
    
    // Verify RAM is visible again
    ASSERT(bus.read8(0x8000) == 0x99);

    std::remove(cartPath);
}
