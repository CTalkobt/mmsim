#include "libcore/main/machine_desc.h"
#include "mmemu_plugin_api.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/isignal_line.h"
#include "libdevices/main/joystick.h"
#include "libcore/main/rom_loader.h"
#include "antic.h"
#include "gtia.h"
#include "pokey.h"
#include "pia6520.h"
#include <iostream>
#include <cstring>
#include <memory>

namespace {

// Signal line that forwards level changes to the CPU IRQ pin.
// IRQ is level-sensitive: the CPU takes an IRQ on every step where irqLine=1
// and I=0, so set(false) MUST clear the CPU's internal irqLine state.
class CpuIrqLine : public ISignalLine {
public:
    explicit CpuIrqLine(ICore* cpu) : m_cpu(cpu) {}
    bool get()  const override { return m_level; }
    void pulse() override { if (m_cpu) m_cpu->triggerIrq(); }
    void set(bool level) override {
        m_level = level;
        if (m_cpu) m_cpu->setIrqLine(level);
    }
private:
    ICore* m_cpu;
    bool   m_level = false;
};

// Signal line that forwards level changes to the CPU NMI pin.
// NMI is edge-triggered: rising edge calls triggerNmi() to force the edge latch.
// Falling edge calls setNmiLine(false) so the CPU's nmiLine tracks the wire.
class CpuNmiLine : public ISignalLine {
public:
    explicit CpuNmiLine(ICore* cpu) : m_cpu(cpu) {}
    bool get()  const override { return m_level; }
    void pulse() override { if (m_cpu) m_cpu->triggerNmi(); }
    void set(bool level) override {
        bool prev = m_level;
        m_level = level;
        if (level && !prev && m_cpu) {
            // Signal line went high - trigger the edge latch in 6502
            m_cpu->triggerNmi();
        }
        if (m_cpu) {
            // Always keep the raw line state up to date
            m_cpu->setNmiLine(level);
        }
    }
private:
    ICore* m_cpu;
    bool   m_level = false;
};

// Signal line that forwards level changes to the CPU halt state.
class CpuHaltLine : public ISignalLine {
public:
    explicit CpuHaltLine(ICore* cpu) : m_cpu(cpu) {}
    bool get()  const override { return m_halt; }
    void set(bool level) override {
        m_halt = level;
        if (m_cpu) m_cpu->setHaltLine(level);
    }
    void pulse() override { set(true); }
private:
    ICore* m_cpu;
    bool   m_halt = false;
};

/**
 * Basic Open Bus handler that returns 0xFF for reads and ignores writes.
 */
class OpenBusHandler : public IOHandler {
public:
    OpenBusHandler(const std::string& name, uint32_t base, uint32_t size)
        : m_name(name), m_base(base), m_size(size) {}
    const char* name()     const override { return m_name.c_str(); }
    uint32_t    baseAddr() const override { return m_base; }
    uint32_t    addrMask() const override { return m_size - 1; }
    bool ioRead (IBus*, uint32_t addr, uint8_t* val) override {
        if (addr < m_base || addr >= m_base + m_size) return false;
        // Return 0xFF for open bus.
        *val = 0xFF;
        return true;
    }
    bool ioWrite(IBus*, uint32_t addr, uint8_t val) override {
        // Return false so the bus might try to write to internal RAM, 
        // BUT we want RAM sizing to fail. If we return false, FlatMemoryBus 
        // will write to its internal m_data, and then ioRead (which we hooked) 
        // will still return 0xFF, so the read-back will fail to match 
        // the written value! This is exactly what we want for RAM sizing.
        return false;
    }
    void reset() override {}
    void tick(uint64_t) override {}
private:
    std::string m_name;
    uint32_t m_base, m_size;
};

} // namespace

const struct SimPluginHostAPI* g_atariHost = nullptr;

/**
 * Unified Atari 8-bit Machine Descriptor Factory
 */
static MachineDescriptor* createAtariImpl(const char* machineId, const char* displayName, 
                                         const char* osRomPath, uint32_t ramSize, bool isXL) {
    auto* desc = new MachineDescriptor();
    desc->machineId = machineId;
    desc->displayName = displayName;
    desc->licenseClass = "proprietary"; 

    // 1. Memory Bus
    auto* bus = new FlatMemoryBus("system", 16); // 64 KB
    desc->buses.push_back({"system", bus});

    // 2. CPU
    ICore* cpu = nullptr;
    if (g_atariHost && g_atariHost->createCore) {
        cpu = g_atariHost->createCore("6502");
    } else {
        cpu = CoreRegistry::instance().createCore("6502");
    }

    if (cpu) {
        cpu->setDataBus(bus);
        cpu->setCodeBus(bus);
        desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 1});
    }

    // 3. I/O Registry
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    // Simulate restricted RAM for 400/800
    if (!isXL) {
        // By default, the whole space is Open Bus for 400/800
        // until we add IO and ROMs.
        // Actually, we should just mark everything above RAM as Open Bus.
        uint32_t openStart = ramSize;
        uint32_t openSize = 0xD000 - openStart; // Up to IO
        if (openSize > 0) {
            io->registerOwnedHandler(new OpenBusHandler("OpenBus-Gap", openStart, openSize));
            bus->addIoRange(openStart, openSize);
        }
    }

    Antic* antic = nullptr;
    GTIA* gtia = nullptr;
    POKEY* pokey = nullptr;
    PIA6520* pia = nullptr;

    if (g_atariHost && g_atariHost->createDevice) {
        antic = (Antic*)g_atariHost->createDevice("antic");
        gtia = (GTIA*)g_atariHost->createDevice("gtia");
        pokey = (POKEY*)g_atariHost->createDevice("pokey");
        pia = (PIA6520*)g_atariHost->createDevice("6520");
    } else {
        antic = (Antic*)DeviceRegistry::instance().createDevice("antic");
        gtia = (GTIA*)DeviceRegistry::instance().createDevice("gtia");
        pokey = (POKEY*)DeviceRegistry::instance().createDevice("pokey");
        pia = (PIA6520*)DeviceRegistry::instance().createDevice("6520");
    }

    if (!antic) antic = new Antic();
    if (!gtia)  gtia = new GTIA();
    if (!pokey) pokey = new POKEY();
    if (!pia)   pia = new PIA6520("PIA", 0xD300);

    pia->setBaseAddr(0xD300);
    pia->setSwappedRS(true);
    antic->setDmaBus(bus);
    antic->setGtia(gtia); 
    io->registerOwnedHandler(antic);
    io->registerOwnedHandler(gtia);
    // io->registerOwnedHandler(pokey);
    io->registerOwnedHandler(pia);

    // Wire CPU reads/writes through the IO registry
    bus->addIoRange(0xD000, 0x0100); // GTIA
    bus->addIoRange(0xD200, 0x0100); // POKEY
    bus->addIoRange(0xD300, 0x0100); // PIA
    bus->addIoRange(0xD400, 0x0100); // ANTIC
    bus->setIoHooks(
        [io](IBus* b, uint32_t addr, uint8_t* val) { 
            return io->dispatchRead(b, addr, val);
        },
        [io](IBus* b, uint32_t addr, uint8_t  val) { return io->dispatchWrite(b, addr, val); }
    );

    // IRQ/NMI wiring
    if (cpu) {
        auto* irqLine = new CpuIrqLine(cpu);
        auto* nmiLine = new CpuNmiLine(cpu);
        pokey->setIrqLine(irqLine);
        antic->setNmiLine(nmiLine);
        pia->setIrqALine(irqLine);
        pia->setIrqBLine(irqLine);
        
        auto* haltLine = new CpuHaltLine(cpu);
        antic->setHaltLine(haltLine);

        desc->deleters.push_back([irqLine, nmiLine, haltLine]() {
            delete irqLine;
            delete nmiLine;
            delete haltLine;
        });
    }

    // ROMs
    size_t osSize = isXL ? 16384 : 10240;
    uint8_t* osRom = new uint8_t[osSize];
    uint8_t* basicRom = new uint8_t[8192];
    uint8_t* charRom = new uint8_t[2048];
    desc->roms.push_back(osRom);
    desc->roms.push_back(basicRom);
    desc->roms.push_back(charRom);

    if (romLoad(osRomPath, osRom, osSize)) {
        if (isXL) {
            bus->addOverlay(0xC000, 16384, osRom, false);
        } else {
            // Non-XL OS is 10KB. Usually structured as:
            // 2KB FPP ($D800-$DFFF) at offset 0
            // 8KB OS  ($E000-$FFFF) at offset 0x0800
            bus->addOverlay(0xD800, 2048, osRom, false);
            bus->addOverlay(0xE000, 8192, osRom + 0x0800, false);
        }
    } else {
        std::cerr << "Atari: Failed to load OS ROM: " << osRomPath << std::endl;
    }

    if (romLoad("roms/atari/basic.bin", basicRom, 8192)) {
        // Map BASIC for all models if loaded.
        // On 400/800 it's like an always-inserted cartridge.
        bus->addOverlay(0xA000, 8192, basicRom, false);
    }

    if (isXL && romLoad("roms/atari/char.bin", charRom, 2048)) {
        bus->addOverlay(0xE000, 2048, charRom, false);
    }

    if (isXL && pia) {
        struct BankingState {
            bool os = true;
            bool basic = true;
        };
        auto bankingState = std::make_shared<BankingState>();

        pia->setPortBWriteCallback([bus, osRom, basicRom, bankingState, pia](uint8_t val) {
            uint8_t ddr = pia->getDdrb();
            uint8_t effective = val & ddr;
            
            // Bit 0: OS ROM enable (0=enabled, 1=disabled)
            // Bit 1: BASIC ROM enable (0=enabled, 1=disabled)
            bool osEnabled = (effective & 0x01) == 0;
            bool basicEnabled = (effective & 0x02) == 0;

            // HACK: Some XL OS versions write $FF to PORTB during init.
            // If we faithfully bank out the OS, we crash.
            // Real hardware might have a delay or different behavior.
            // For now, let's keep OS enabled if DDR is all outputs.
            if (ddr == 0xFF) osEnabled = true;
            
            if (osEnabled == bankingState->os && basicEnabled == bankingState->basic) return;

            if (osEnabled != bankingState->os) {
                bus->removeRomOverlay(0xC000);
                if (osEnabled) {
                    bus->addOverlay(0xC000, 16384, osRom, false);
                }
                bankingState->os = osEnabled;
            }

            if (basicEnabled != bankingState->basic) {
                bus->removeRomOverlay(0xA000);
                if (basicEnabled) {
                    bus->addOverlay(0xA000, 8192, basicRom, false);
                }
                bankingState->basic = basicEnabled;
            }
        });
    }

    desc->onReset = [pia](MachineDescriptor& d) {
        if (d.ioRegistry) {
            d.ioRegistry->resetAll();
            // Ensure PIA Port B starts in a state that enables OS/BASIC
            if (pia) {
                pia->ioWrite(nullptr, 0xD303, 0x00); // DDRB access
                pia->ioWrite(nullptr, 0xD301, 0xFF); // All outputs
                pia->ioWrite(nullptr, 0xD303, 0x04); // Port access
                pia->ioWrite(nullptr, 0xD301, 0xFC); // OS ON, BASIC ON
            }
        }
        for (auto& slot : d.cpus) {
            if (slot.cpu) slot.cpu->reset();
        }
    };

    desc->schedulerStep = [](MachineDescriptor& d) {
        int totalCycles = 0;
        for (auto& slot : d.cpus) {
            if (slot.active && slot.cpu) {
                int cycles = slot.cpu->step();
                if (cycles == 0) cycles = 1; 
                totalCycles += cycles;
                if (d.ioRegistry) d.ioRegistry->tickAll(cycles);
            }
        }
        return totalCycles;
    };

    return desc;
}

extern "C" {
    MachineDescriptor* createMachineAtari400() {
        return createAtariImpl("atari400", "Atari 400", "roms/atari/osa.bin", 16384, false);
    }
    MachineDescriptor* createMachineAtari800() {
        return createAtariImpl("atari800", "Atari 800", "roms/atari/osb.bin", 49152, false);
    }
    MachineDescriptor* createMachineAtari800XL() {
        return createAtariImpl("atari800xl", "Atari 800XL", "roms/atari/osxl.bin", 65536, true);
    }
}
