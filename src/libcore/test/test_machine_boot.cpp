#include "test_harness.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/ivideo_output.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/ieee488.h"
#include "cpu6502.h"
#include "cpu6510.h"
#include "pia6520.h"
#include "via6522.h"
#include "crtc6545.h"
#include "pet_video.h"
#include "vic2.h"
#include "c64_pla.h"
#include "cia6526.h"
#include "sid6581.h"
#include "kbd_c64.h"
#include "keyboard_matrix_pet.h"
#include <vector>
#include <string>
#include <cstring>

// Minimal keyboard wrapper for PET tests
struct KbdPetBootTestWrapper : public IOHandler, public IKeyboardMatrix {
    explicit KbdPetBootTestWrapper(PetKeyboardMatrix::Layout l) : m_kbd(l) {}
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
    IPortDevice* getPort(int i) override { (void)i; return &m_kbd; }
    PetKeyboardMatrix m_kbd;
};

static bool s_bootTestRegistriesReady = false;
static void ensureBootTestRegistriesReady() {
    if (s_bootTestRegistriesReady) return;
    s_bootTestRegistriesReady = true;

    // Cores
    CoreRegistry::instance().registerCore("6502", "NMOS", "open",
        []() -> ICore* { return new MOS6502(); });
    CoreRegistry::instance().registerCore("6510", "MOS6510", "open",
        []() -> ICore* { return new MOS6510(); });

    // PET Devices
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
            return new KbdPetBootTestWrapper(PetKeyboardMatrix::Layout::GRAPHICS);
        });
    DeviceRegistry::instance().registerDevice("kbd_pet_business",
        []() -> IOHandler* {
            return new KbdPetBootTestWrapper(PetKeyboardMatrix::Layout::BUSINESS);
        });
    DeviceRegistry::instance().registerDevice("ieee488",
        []() -> IOHandler* { return new SimpleIEEE488Bus(); });

    // C64 Devices
    DeviceRegistry::instance().registerDevice("c64_pla",
        []() -> IOHandler* { return new C64PLA(); });
    DeviceRegistry::instance().registerDevice("6567",
        []() -> IOHandler* { return new VIC2("VIC-II", 0xD000); });
    DeviceRegistry::instance().registerDevice("6581",
        []() -> IOHandler* { return new SID6581("SID", 0xD400); });
    DeviceRegistry::instance().registerDevice("6526",
        []() -> IOHandler* { return new CIA6526("CIA", 0); });
    DeviceRegistry::instance().registerDevice("kbd_c64",
        []() -> IOHandler* { return new KbdC64(); });

    // Load JSONs
    JsonMachineLoader().loadFile("machines/pet.json");
    JsonMachineLoader().loadFile("machines/c64.json");
}

static void ensureRegistriesAndLoad() {
    ensureBootTestRegistriesReady();
}

/**
 * Generic machine boot verification.
 * Replicates the logic of loading a machine, resetting it, and running
 * for a specific number of cycles, then checking for video output.
 */
static void verifyMachineBoot(const std::string& machineId, uint64_t cyclesToRun) {
    ensureRegistriesAndLoad();
    
    auto* desc = MachineRegistry::instance().createMachine(machineId);
    ASSERT(desc != nullptr);
    ASSERT(desc->ioRegistry != nullptr);
    ASSERT(desc->schedulerStep != nullptr);
    ASSERT(desc->onReset != nullptr);

    // Initial reset
    desc->onReset(*desc);

    // Run scheduler
    uint64_t cyclesDone = 0;
    while (cyclesDone < cyclesToRun) {
        cyclesDone += desc->schedulerStep(*desc);
    }

    // Find Video Output
    IVideoOutput* video = nullptr;
    IOHandler* videoHandler = nullptr;
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    for (auto* h : handlers) {
        if (dynamic_cast<IVideoOutput*>(h)) {
            video = dynamic_cast<IVideoOutput*>(h);
            videoHandler = h;
            break;
        }
    }
    
    ASSERT(video != nullptr);

    // Diagnostic: check if Char ROM is null (using peek8 won't work, we need internal access or rely on rendering)
    // We can't easily check m_charRom from here without making it public or adding an accessor.
    // However, if we suspect it's null, we'll see it in the behavior.
    
    auto dims = video->getDimensions();
    ASSERT(dims.width > 0 && dims.height > 0);
    
    std::vector<uint32_t> frame(dims.width * dims.height);
    video->renderFrame(frame.data());
    
    // Verify frame is not a solid single color (implies background only)
    uint32_t firstPixel = frame[0];
    bool foundContent = false;
    for (uint32_t p : frame) {
        if (p != firstPixel) {
            foundContent = true;
            break;
        }
    }
    
    if (!foundContent) {
        fprintf(stderr, "Boot verification failed for %s: solid color frame detected after %lu cycles.\n", 
                machineId.c_str(), cyclesDone);
        // Dump some CPU state
        if (!desc->cpus.empty()) {
            ICore* cpu = desc->cpus[0].cpu;
            fprintf(stderr, "CPU PC: $%04X, Cycles: %lu\n", cpu->pc(), cpu->cycles());
        }
        // Dump VRAM snippet
        if (machineId == "c64") {
            fprintf(stderr, "VRAM at $0400: ");
            for (int i=0; i<16; ++i) fprintf(stderr, "%02X ", desc->buses[0].bus->peek8(0x0400+i));
        } else {
            fprintf(stderr, "VRAM at $8000: ");
            for (int i=0; i<16; ++i) fprintf(stderr, "%02X ", desc->buses[0].bus->peek8(0x8000+i));
        }
        fprintf(stderr, "\n");
    }
    ASSERT(foundContent);

    delete desc;
}

TEST_CASE(pet2001_boot_screen) {
    // 5 seconds @ 1MHz should be more than enough for PET boot
    verifyMachineBoot("pet2001", 5000000);
}

TEST_CASE(c64_boot_screen) {
    // 5 seconds is ample for C64 Kernal to initialize and display message
    verifyMachineBoot("c64", 5000000);
}
