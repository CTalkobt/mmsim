#include "test_harness.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/iport_device.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/ieee488.h"
#include "cpu6502.h"
#include "pia6520.h"
#include "via6522.h"
#include "crtc6545.h"
#include "pet_video.h"
#include "keyboard_matrix_pet.h"
#include <cstring>

// Minimal keyboard wrapper so the test binary can create kbd_pet devices
// without loading the full PET plugin .so.
struct KbdPetTestWrapper : public IOHandler, public IKeyboardMatrix {
    explicit KbdPetTestWrapper(PetKeyboardMatrix::Layout l) : m_kbd(l) {}
    const char* name()     const override { return "kbd_pet"; }
    uint32_t    baseAddr() const override { return 0; }
    uint32_t    addrMask() const override { return 0; }
    bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
    void reset() override {}
    void tick(uint64_t) override {}
    void keyDown(int, int) override {}
    void keyUp(int, int)   override {}
    void clearKeys() override {}
    bool pressKeyByName(const std::string& n, bool d) override {
        if (d) m_kbd.keyDown(n); else m_kbd.keyUp(n); return true;
    }
    IPortDevice* getPort(int) override { return nullptr; }
    PetKeyboardMatrix m_kbd;
};

static bool s_petRegistriesReady = false;
static void ensureRegistriesReady() {
    if (s_petRegistriesReady) return;
    s_petRegistriesReady = true;
    CoreRegistry::instance().registerCore("6502", "NMOS", "open",
        []() -> ICore* { return new MOS6502(); });
    DeviceRegistry::instance().registerDevice("6520",
        []() -> IOHandler* { return new PIA6520(); });
    DeviceRegistry::instance().registerDevice("6522",
        []() -> IOHandler* { return new VIA6522("VIA", 0); });
    DeviceRegistry::instance().registerDevice("6545",
        []() -> IOHandler* { return new CRTC6545(); });
    DeviceRegistry::instance().registerDevice("pet_video_2001",
        []() -> IOHandler* { return new PetVideo(PetVideo::Model::PET_2001); });
    DeviceRegistry::instance().registerDevice("pet_video_4000",
        []() -> IOHandler* { return new PetVideo(PetVideo::Model::PET_4000); });
    DeviceRegistry::instance().registerDevice("pet_video_8000",
        []() -> IOHandler* { return new PetVideo(PetVideo::Model::PET_8000); });
    DeviceRegistry::instance().registerDevice("kbd_pet_graphics",
        []() -> IOHandler* {
            return new KbdPetTestWrapper(PetKeyboardMatrix::Layout::GRAPHICS);
        });
    DeviceRegistry::instance().registerDevice("kbd_pet_business",
        []() -> IOHandler* {
            return new KbdPetTestWrapper(PetKeyboardMatrix::Layout::BUSINESS);
        });
    JsonMachineLoader().loadFile("machines/pet.json");
}

TEST_CASE(PET_2001_Setup) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("pet2001");
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet2001");

    auto* bus = desc->buses[0].bus;
    // Test RAM
    bus->write8(0x1000, 0x42);
    ASSERT(bus->read8(0x1000) == 0x42);

    // Test Video RAM
    bus->write8(0x8000, 0x55);
    ASSERT(bus->read8(0x8000) == 0x55);
    delete desc;
}

TEST_CASE(PET_4032_Setup) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("pet4032");
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet4032");
    delete desc;
}

TEST_CASE(PET_8032_Setup) {
    ensureRegistriesReady();
    auto* desc = MachineRegistry::instance().createMachine("pet8032");
    ASSERT(desc != nullptr);
    ASSERT(desc->machineId == "pet8032");
    delete desc;
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
