#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/isignal_line.h"
#include "libdevices/main/joystick.h"
#include "libcore/main/rom_loader.h"
#include "via6522.h"
#include "vic6560.h"
#include <iostream>
#include <cstring>

// Signal line that forwards level changes to the CPU IRQ pin.
class CpuIrqLine : public ISignalLine {
public:
    explicit CpuIrqLine(ICore* cpu) : m_cpu(cpu) {}
    bool get()  const override { return m_level; }
    void pulse() override { if (m_cpu) m_cpu->triggerIrq(); }
    void set(bool level) override {
        m_level = level;
        if (level && m_cpu) m_cpu->triggerIrq();
    }
private:
    ICore* m_cpu;
    bool   m_level = false;
};

// Expansion RAM block descriptors
struct RamBlock { uint32_t base; uint32_t size; };
static constexpr RamBlock VIC20_BLK0 = {0x0400, 0x0C00}; // +3K
static constexpr RamBlock VIC20_BLK1 = {0x2000, 0x2000}; // +8K
static constexpr RamBlock VIC20_BLK2 = {0x4000, 0x2000}; // +8K
static constexpr RamBlock VIC20_BLK3 = {0x6000, 0x2000}; // +8K
static constexpr RamBlock VIC20_BLK5 = {0xA000, 0x2000}; // +8K

static constexpr uint32_t EXP_BLK0 = 1u << 0;
static constexpr uint32_t EXP_BLK1 = 1u << 1;
static constexpr uint32_t EXP_BLK2 = 1u << 2;
static constexpr uint32_t EXP_BLK3 = 1u << 3;
static constexpr uint32_t EXP_BLK5 = 1u << 4;

// Open-bus handler: covers an unmapped address range, returns 0xFF on reads.
class OpenBusHandler : public IOHandler {
public:
    OpenBusHandler(const char* n, uint32_t base, uint32_t size)
        : m_name(n), m_base(base), m_size(size) {}
    const char* name()     const override { return m_name; }
    uint32_t    baseAddr() const override { return m_base; }
    uint32_t    addrMask() const override { return m_size - 1; }
    bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
        if (addr < m_base || addr >= m_base + m_size) return false;
        *val = 0xFF;
        return true;
    }
    bool ioWrite(IBus*, uint32_t addr, uint8_t) override {
        return addr >= m_base && addr < m_base + m_size;
    }
    void reset() override {}
    void tick(uint64_t) override {}
private:
    const char* m_name;
    uint32_t m_base;
    uint32_t m_size;
};

// Register Color RAM as a simple IOHandler so it's accessible via bus
class ColorRamHandler : public IOHandler {
public:
    ColorRamHandler(uint8_t* ram) : m_ram(ram) {}
    const char* name() const override { return "ColorRAM"; }
    uint32_t baseAddr() const override { return 0x9400; }
    uint32_t addrMask() const override { return 0x03FF; }
    bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
        if ((addr & ~addrMask()) != baseAddr()) return false;
        *val = m_ram[addr & 0x03FF] | 0xF0; // Only low 4 bits
        return true;
    }
    bool ioWrite(IBus*, uint32_t addr, uint8_t val) override {
        if ((addr & ~addrMask()) != baseAddr()) return false;
        m_ram[addr & 0x03FF] = val & 0x0F;
        return true;
    }
    void reset() override {}
    void tick(uint64_t) override {}
private:
    uint8_t* m_ram;
};

/**
 * VIC-20 Machine Descriptor Factory — shared implementation.
 * @param expansionFlags  Bitmask of EXP_BLKx constants for fitted RAM expansions.
 * @param machineId       Registry ID (e.g. "vic20+8k").
 * @param displayName     Human-readable name.
 */
static MachineDescriptor* createMachineVic20Impl(uint32_t expansionFlags,
                                                  const char* machineId,
                                                  const char* displayName) {
    auto* desc = new MachineDescriptor();
    desc->machineId = machineId;
    desc->displayName = displayName;
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
    } else {
        std::cerr << "Error: VIC-20 could not find '6502' core." << std::endl;
    }

    // 3. I/O Devices
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    auto* vic = (VIC6560*)DeviceRegistry::instance().createDevice("6560");
    
    // 3.5 Color RAM (1K x 4 bits, mapped at $9400 or $9600 depending on RAM expansion)
    uint8_t* colorRam = new uint8_t[1024];
    std::memset(colorRam, 0, 1024);
    desc->roms.push_back(colorRam);

    // Create VIA devices from registry (if available)
    auto* via1 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");
    auto* via2 = (VIA6522*)DeviceRegistry::instance().createDevice("6522");

    if (!vic) {
        std::cerr << "VIC-20: VIC-I plugin not found, creating internal." << std::endl;
        vic = new VIC6560("VIC-I", 0x9000);
    } else {
        vic->setName("VIC-I");
        vic->setBaseAddr(0x9000);
    }
    vic->setBus(bus);
    vic->setColorRam(colorRam);

    if (!via1 || !via2) {
        std::cerr << "VIC-20: VIA plugin not found, creating internal." << std::endl;
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

    // Wire CPU reads/writes through the IO registry so VIC/VIA registers are
    // actually accessed instead of flat RAM.
    bus->setIoHooks(
        [io, bus](uint32_t addr, uint8_t* val) { return io->dispatchRead(bus, addr, val); },
        [io, bus](uint32_t addr, uint8_t  val) { return io->dispatchWrite(bus, addr, val); }
    );

    // Wire VIA IRQ lines to the CPU so timer / peripheral interrupts are delivered.
    // Both VIAs share one line (open-collector, wired-OR).
    if (cpu) {
        auto* irqLine = new CpuIrqLine(cpu);
        via1->setIrqLine(irqLine);
        via2->setIrqLine(irqLine);
    }

    // 3.6 Keyboard Matrix — wired as two IPortDevices into the VIAs.
    // Registered in the IORegistry so resetAll() and tickAll() reach it.
    IOHandler* kbdHandler = DeviceRegistry::instance().createDevice("kbd_vic20");
    if (kbdHandler) {
        IKeyboardMatrix* kbd = dynamic_cast<IKeyboardMatrix*>(kbdHandler);
        if (kbd) {
            via2->setPortBDevice(kbd->getPort(0)); // ColumnPort → VIA2 Port B (column select)
            via2->setPortADevice(kbd->getPort(1)); // RowPort   → VIA2 Port A (row sense)
            io->registerHandler(kbdHandler);
            desc->onKey = [kbd](const std::string& keyName, bool down) {
                return kbd->pressKeyByName(keyName, down);
            };
        }
    } else {
        std::cerr << "Error: VIC-20 could not find 'kbd_vic20' device." << std::endl;
    }

    io->registerHandler(new ColorRamHandler(colorRam));

    // 3.7 Joystick
    auto* joy = new Joystick();
    via1->setPortBDevice(joy);
    desc->onJoystick = [joy](int port, uint8_t bits) {
        if (port == 0) joy->setState(bits);
    };

    // 4. ROM Overlays
    uint8_t* charRom = new uint8_t[4096];
    uint8_t* basicRom = new uint8_t[8192];
    uint8_t* kernalRom = new uint8_t[8192];

    desc->roms.push_back(charRom);
    desc->roms.push_back(basicRom);
    desc->roms.push_back(kernalRom);

    if (romLoad("roms/vic20/char.bin", charRom, 4096)) {
        bus->addOverlay(0x8000, 4096, charRom, false);
    } else {
        std::cerr << "VIC-20: Warning: char.bin not found." << std::endl;
    }

    if (romLoad("roms/vic20/basic.bin", basicRom, 8192)) {
        bus->addOverlay(0xC000, 8192, basicRom, false);
    } else {
        std::cerr << "VIC-20: Warning: basic.bin not found." << std::endl;
    }

    if (romLoad("roms/vic20/kernal.bin", kernalRom, 8192)) {
        bus->addOverlay(0xE000, 8192, kernalRom, false);
    } else {
        std::cerr << "VIC-20: Warning: kernal.bin not found." << std::endl;
    }

    // 5. Mark expansion blocks that are NOT fitted as open bus so that
    //    reads from those ranges return 0xFF instead of flat backing RAM.
    struct { uint32_t flag; RamBlock blk; const char* lbl; } expansionBlocks[] = {
        {EXP_BLK0, VIC20_BLK0, "OpenBus-BLK0"},
        {EXP_BLK1, VIC20_BLK1, "OpenBus-BLK1"},
        {EXP_BLK2, VIC20_BLK2, "OpenBus-BLK2"},
        {EXP_BLK3, VIC20_BLK3, "OpenBus-BLK3"},
        {EXP_BLK5, VIC20_BLK5, "OpenBus-BLK5"},
    };
    for (const auto& e : expansionBlocks) {
        if (!(expansionFlags & e.flag)) {
            io->registerHandler(new OpenBusHandler(e.lbl, e.blk.base, e.blk.size));
        }
    }

    desc->onReset = [](MachineDescriptor& d) {
        if (d.ioRegistry) d.ioRegistry->resetAll(); // resets all devices incl. keyboard
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

// Public factory entry points — one per RAM configuration.
MachineDescriptor* createMachineVic20() {
    return createMachineVic20Impl(0,
        "vic20", "Commodore VIC-20");
}
MachineDescriptor* createMachineVic20_3K() {
    return createMachineVic20Impl(EXP_BLK0,
        "vic20+3k", "Commodore VIC-20 +3K");
}
MachineDescriptor* createMachineVic20_8K() {
    return createMachineVic20Impl(EXP_BLK1,
        "vic20+8k", "Commodore VIC-20 +8K");
}
MachineDescriptor* createMachineVic20_16K() {
    return createMachineVic20Impl(EXP_BLK1 | EXP_BLK2,
        "vic20+16k", "Commodore VIC-20 +16K");
}
MachineDescriptor* createMachineVic20_32K() {
    return createMachineVic20Impl(EXP_BLK1 | EXP_BLK2 | EXP_BLK3 | EXP_BLK5,
        "vic20+32k", "Commodore VIC-20 +32K");
}
