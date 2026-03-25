#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "via6522.h"
#include "vic6560.h"
#include <iostream>
#include <cstring>

/**
 * VIC-20 Machine Descriptor Factory.
 */
MachineDescriptor* createMachineVic20() {
    auto* desc = new MachineDescriptor();
    desc->machineId = "vic20";
    desc->displayName = "Commodore VIC-20";
    desc->licenseClass = "proprietary"; // ROMs are proprietary

    // 1. Memory Bus
    auto* bus = new FlatMemoryBus("system", 16); // 64 KB
    desc->buses.push_back({"system", bus});

    // 2. CPU
    ICore* cpu = CoreRegistry::instance().createCore("6502");
    if (cpu) {
        cpu->setDataBus(bus);
        cpu->setCodeBus(bus);
        desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 1});
    }

    // 3. I/O Devices
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    auto* vic = (VIC6560*)DeviceRegistry::instance().createDevice("6560");
    
    // 3.5 Color RAM (1K x 4 bits, mapped at $9400 or $9600 depending on RAM expansion)
    uint8_t* colorRam = new uint8_t[1024];
    std::memset(colorRam, 0, 1024);

    // Create VIA devices from registry (if available)
    auto* via1 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");
    auto* via2 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");

    if (!vic) vic = new VIC6560("VIC-I", 0x9000);
    else {
        vic->setName("VIC-I");
        vic->setBaseAddr(0x9000);
    }
    vic->setColorRam(colorRam);

    if (!via1 || !via2) {
        // Fallback to direct creation if plugin not loaded or internal
        if (!via1) via1 = new VIA6522("VIA1", 0x9110);
        if (!via2) via2 = new VIA6522("VIA2", 0x9120);
    } else {
        via1->setName("VIA1");
        via1->setBaseAddr(0x9110);
        via2->setName("VIA2");
        via2->setBaseAddr(0x9120);
    }

    io->registerHandler(vic);
    io->registerHandler(via1);
    io->registerHandler(via2);

    // Register Color RAM as a simple IOHandler so it's accessible via bus
    class ColorRamHandler : public IOHandler {
    public:
        ColorRamHandler(uint8_t* ram) : m_ram(ram) {}
        std::string name() const override { return "ColorRAM"; }
        uint32_t baseAddr() const override { return 0x9400; }
        uint32_t addrMask() const override { return 0x03FF; }
        bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
            *val = m_ram[addr & 0x03FF] | 0xF0; // Only low 4 bits
            return true;
        }
        bool ioWrite(IBus*, uint32_t addr, uint8_t val) override {
            m_ram[addr & 0x03FF] = val & 0x0F;
            return true;
        }
        void reset() override {}
        void tick(uint64_t) override {}
    private:
        uint8_t* m_ram;
    };
    io->registerHandler(new ColorRamHandler(colorRam));

    // TODO: Wire interrupts (VIA -> CPU IRQ)

    // 4. ROM Overlays (Placeholders)
    // desc->overlays.push_back({"roms/vic20/char.bin", 0x8000, true, true});
    // desc->overlays.push_back({"roms/vic20/basic.bin", 0xC000, true, true});
    // desc->overlays.push_back({"roms/vic20/kernal.bin", 0xE000, true, true});

    desc->onReset = [](MachineDescriptor& d) {
        if (d.ioRegistry) d.ioRegistry->resetAll();
        for (auto& slot : d.cpus) {
            if (slot.cpu) slot.cpu->reset();
        }
    };

    desc->schedulerStep = [](MachineDescriptor& d) {
        int totalCycles = 0;
        for (auto& slot : d.cpus) {
            if (slot.active && slot.cpu) {
                int cycles = slot.cpu->step();
                totalCycles += cycles;
                if (d.ioRegistry) d.ioRegistry->tickAll(cycles);
            }
        }
        return totalCycles;
    };

    return desc;
}
