#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/ibus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/ivideo_output.h"
#include "mmemu_plugin_api.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include "plugins/6502/main/cpu6502.h"

// Defined in machine_atari.cpp
extern "C" MachineDescriptor* createMachineAtari400();
extern "C" MachineDescriptor* createMachineAtari800();
extern "C" MachineDescriptor* createMachineAtari800XL();

static IBus* getBus(MachineDescriptor* desc) {
    for (auto& b : desc->buses) {
        if (b.busName == "system") return b.bus;
    }
    return nullptr;
}

static ICore* createCore6502() {
    return new MOS6502();
}

static void run_atari_boot_debug(MachineDescriptor* desc, const std::string& name) {
    auto* bus = getBus(desc);
    auto* cpu = desc->cpus[0].cpu;

    uint16_t resetVec = bus->peek8(0xFFFC) | (bus->peek8(0xFFFD) << 8);
    std::cout << "--- Debugging Boot for " << name << " ---" << std::endl;
    std::cout << "Reset Vector: $" << std::hex << resetVec << std::endl;

    if (desc->onReset) desc->onReset(*desc);

    // Run for a bit and log PC
    for (int i = 0; i < 50000000; ++i) { // 50M steps
        uint16_t pc = (uint16_t)cpu->regReadByName("PC");
        if (i % 10000000 == 0) {
            uint8_t z14 = bus->read8(0x14);
            std::cout << "Step " << i << " PC=$" << std::hex << pc << " RTCLOK=$" << (int)z14 << std::endl;
        }
        desc->schedulerStep(*desc);
        
        if (name == "Atari 800" && i == 5000000) {
            std::cout << "DEBUG: Spoofing keypress (Return) at step 5M" << std::endl;
            // Write to CH ($02FC) and set KBD interrupt bit in NMIST ($D40F)
            bus->write8(0x02FC, 0x0C); // Return keycode
            // Actually the OS checks SKSTAT bit 2 ($04) for key waiting
            // We don't have a direct hook yet, but let's try setting the shadow.
        }
    }

    uint16_t finalPc = (uint16_t)cpu->regReadByName("PC");
    std::cout << "PC after 50000000 steps: $" << std::hex << finalPc << std::endl;

    // Test rendering
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    IVideoOutput* vid = nullptr;
    for (auto* h : handlers) {
        if (std::string(h->name()) == "ANTIC") {
            vid = dynamic_cast<IVideoOutput*>(h);
            break;
        }
    }

    if (vid) {
        std::cout << "DEBUG: Calling renderFrame..." << std::endl;
        auto dim = vid->getDimensions();
        std::vector<uint32_t> pixels(dim.width * dim.height);
        vid->renderFrame(pixels.data());
        
        uint32_t bg = pixels[0];
        bool nonBg = false;
        for (auto p : pixels) {
            if (p != bg) {
                nonBg = true;
                break;
            }
        }
        std::cout << "DEBUG: renderFrame completed. Non-background pixels found: " << (nonBg ? "YES" : "NO") << std::endl;
    }

    delete desc;
}

int main() {
    CoreRegistry::instance().registerCore("6502", "NMOS", "open", createCore6502);
    run_atari_boot_debug(createMachineAtari800(), "Atari 800");
    run_atari_boot_debug(createMachineAtari800XL(), "Atari 800XL");
    return 0;
}
