#include "test_harness.h"
#include "libcore/main/rom_loader.h"
#include "libcore/main/machine_desc.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"

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

TEST_CASE(machine_descriptor_cleanup) {
    auto* desc = new MachineDescriptor();
    desc->machineId = "test";
    
    auto* bus = new FlatMemoryBus("test", 16);
    desc->buses.push_back({"system", bus});
    
    auto* io = new IORegistry();
    desc->ioRegistry = io;
    
    class MockHandler : public IOHandler {
    public:
        MockHandler(bool* deleted) : m_deleted(deleted) {}
        ~MockHandler() override { *m_deleted = true; }
        const char* name() const override { return "mock"; }
        uint32_t baseAddr() const override { return 0; }
        uint32_t addrMask() const override { return 0; }
        bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
        bool ioWrite(IBus*, uint32_t, uint8_t) override { return false; }
        void reset() override {}
        void tick(uint64_t) override {}
    private:
        bool* m_deleted;
    };
    
    bool handlerDeleted = false;
    io->registerOwnedHandler(new MockHandler(&handlerDeleted));
    
    uint8_t* rom = new uint8_t[1024];
    desc->roms.push_back(rom);
    
    bool genericDeleted = false;
    desc->deleters.push_back([&genericDeleted]() { genericDeleted = true; });
    
    delete desc;
    
    ASSERT(handlerDeleted == true);
    ASSERT(genericDeleted == true);
    // Note: cpus and buses are also deleted, but they are pointers to ICore/IBus
    // which we'd need more mocks to verify. This is enough for now.
}
