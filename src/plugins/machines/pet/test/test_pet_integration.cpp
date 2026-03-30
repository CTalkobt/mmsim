#include "test_harness.h"
#include "libcore/main/machine_desc.h"
#include "plugins/machines/pet/main/machine_pet.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/ieee488.h"
#include <cstring>

// Forward declarations
MachineDescriptor* createPet2001();
MachineDescriptor* createPet4032();
MachineDescriptor* createPet8032();

TEST_CASE(PET_2001_Setup) {
    auto* desc = createPet2001();
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet2001");
    
    auto* bus = desc->buses[0].bus;
    // Test RAM
    bus->write8(0x1000, 0x42);
    ASSERT(bus->read8(0x1000) == 0x42);
    
    // Test Video RAM
    bus->write8(0x8000, 0x55);
    ASSERT(bus->read8(0x8000) == 0x55);
}

TEST_CASE(PET_4032_Setup) {
    auto* desc = createPet4032();
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet4032");
}

TEST_CASE(PET_8032_Setup) {
    auto* desc = createPet8032();
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet8032");
}

TEST_CASE(IEEE488_Bus_Smoke) {
    SimpleIEEE488Bus bus;
    
    // Initial state: all high (logic 0)
    ASSERT(bus.getSignal(IEEE488Bus::ATN) == true);
    
    bus.setSignal(IEEE488Bus::ATN, false);
    ASSERT(bus.getSignal(IEEE488Bus::ATN) == false);
    
    bus.setData(0xAA);
    ASSERT(bus.getData() == 0xAA);
}
