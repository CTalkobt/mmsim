#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/isignal_line.h"
#include "libcore/main/rom_loader.h"
#include "cpu6510.h"
#include "c64_pla.h"
#include "cia6526.h"
#include "vic2.h"
#include "sid6581.h"
#include <iostream>
#include <cstring>
#include <map>
#include <string>
#include <algorithm>

namespace {

// ---------------------------------------------------------------------------
// Signal line implementations
// ---------------------------------------------------------------------------

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

class CpuNmiLine : public ISignalLine {
public:
    explicit CpuNmiLine(ICore* cpu) : m_cpu(cpu) {}
    bool get()  const override { return m_level; }
    void pulse() override { if (m_cpu) m_cpu->triggerNmi(); }
    void set(bool level) override {
        m_level = level;
        if (level && m_cpu) m_cpu->triggerNmi();
    }
private:
    ICore* m_cpu;
    bool   m_level = false;
};

// ---------------------------------------------------------------------------
// Color RAM: 1 KB × 4 bits at $D800–$DBFF.
// Upper nibble always reads as 1s (real hardware: upper 4 bits are floating).
// ---------------------------------------------------------------------------

class ColorRamHandler : public IOHandler {
public:
    explicit ColorRamHandler(uint8_t* ram) : m_ram(ram) {}
    const char* name()     const override { return "ColorRAM"; }
    uint32_t    baseAddr() const override { return 0xD800; }
    uint32_t    addrMask() const override { return 0x03FF; }
    bool ioRead(IBus*, uint32_t addr, uint8_t* val) override {
        if ((addr & ~addrMask()) != baseAddr()) return false;
        *val = m_ram[addr & 0x03FF] | 0xF0;
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

// ---------------------------------------------------------------------------
// C64 Keyboard Matrix
//
// Wired to CIA1: Port A ($DC00) = column select (CPU output, active low),
//                Port B ($DC01) = row sense (CPU input, active low).
//
// Key matrix — m_matrix[PA_bit] holds PB row bitmask (0=pressed).
// Pairs in s_keyMap are {PB_bit, PA_bit}.
//
//           PA0      PA1    PA2      PA3   PA4    PA5   PA6      PA7
// PB0:     DEL      3      5        7     9      +     POUND    1
// PB1:     RET      W      R        Y     I      P     *        ARROW_LT
// PB2:     CRSR_RT  A      D        G     J      L     ;        CTRL
// PB3:     F7       4      6        8     0      -     HOME     2
// PB4:     F1       Z      C        B     M      .     R_SHFT   SPACE
// PB5:     F3       S      F        H     K      :     =        CBM
// PB6:     F5       E      T        U     O      @     ARROW_UP Q
// PB7:     CRSR_DN  LSHFT  X        V     N      ,     /        RUN_STOP
// ---------------------------------------------------------------------------

class KbdC64 : public IOHandler {
public:
    KbdC64() : m_colPort(this), m_rowPort(this) { clearKeys(); }
    ~KbdC64() override = default;

    // -----------------------------------------------------------------------
    // IOHandler — null handler; registered so reset() clears keys on machine reset
    // -----------------------------------------------------------------------
    const char* name()     const override { return "C64Keyboard"; }
    uint32_t    baseAddr() const override { return 0; }
    uint32_t    addrMask() const override { return 0; }
    bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
    void reset() override { clearKeys(); }
    void tick(uint64_t) override {}

    // -----------------------------------------------------------------------
    // Keyboard matrix interface
    // -----------------------------------------------------------------------

    void clearKeys() {
        std::memset(m_matrix, 0xFF, sizeof(m_matrix));
        updateMatrix();
    }

    bool pressKeyByName(const std::string& keyName, bool down) {
        std::string lower = keyName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        auto it = s_keyMap.find(lower);
        if (it == s_keyMap.end()) return false;
        int pa = it->second.second;  // PA bit: which column is scanned
        int pb = it->second.first;   // PB bit: which row goes low
        if (down) m_matrix[pa] &= ~(1 << pb);
        else      m_matrix[pa] |=  (1 << pb);
        updateMatrix();
        return true;
    }

    // -----------------------------------------------------------------------
    // Port devices — attach these to CIA1 Port A and Port B
    // -----------------------------------------------------------------------

    /** CIA1 Port A device: CPU writes column-select here. */
    class ColumnPort : public IPortDevice {
    public:
        explicit ColumnPort(KbdC64* p) : m_parent(p) {}
        uint8_t readPort() override { return m_val; }
        void    writePort(uint8_t v) override { m_val = v; m_parent->updateMatrix(); }
        void    setDdr(uint8_t) override {}
        uint8_t m_val = 0xFF;
    private:
        KbdC64* m_parent;
    };

    /** CIA1 Port B device: CPU reads row-sense here. */
    class RowPort : public IPortDevice {
    public:
        explicit RowPort(KbdC64* p) : m_parent(p) {}
        uint8_t readPort() override { return m_parent->m_rowVal; }
        void    writePort(uint8_t) override {}
        void    setDdr(uint8_t) override {}
    private:
        KbdC64* m_parent;
    };

    IPortDevice* getColumnPort() { return &m_colPort; }
    IPortDevice* getRowPort()    { return &m_rowPort; }

private:
    void updateMatrix() {
        uint8_t rows = 0xFF;
        for (int col = 0; col < 8; ++col)
            if (!(m_colPort.m_val & (1 << col)))
                rows &= m_matrix[col];
        m_rowVal = rows;
    }

    uint8_t      m_matrix[8];
    uint8_t      m_rowVal = 0xFF;
    ColumnPort   m_colPort;
    RowPort      m_rowPort;

    static std::map<std::string, std::pair<int,int>> s_keyMap;
};

std::map<std::string, std::pair<int,int>> KbdC64::s_keyMap = {
    // PB row 0
    {"delete",{0,0}},   {"3",{0,1}},    {"5",{0,2}},   {"7",{0,3}},
    {"9",{0,4}},        {"plus",{0,5}}, {"pound",{0,6}},{"1",{0,7}},
    // PB row 1
    {"return",{1,0}},   {"w",{1,1}},    {"r",{1,2}},   {"y",{1,3}},
    {"i",{1,4}},        {"p",{1,5}},    {"asterisk",{1,6}},{"arrow_left",{1,7}},
    // PB row 2
    {"crsr_right",{2,0}},{"a",{2,1}},  {"d",{2,2}},   {"g",{2,3}},
    {"j",{2,4}},        {"l",{2,5}},    {"semicolon",{2,6}},{"ctrl",{2,7}},
    // PB row 3
    {"f7",{3,0}},       {"4",{3,1}},    {"6",{3,2}},   {"8",{3,3}},
    {"0",{3,4}},        {"minus",{3,5}},{"home",{3,6}}, {"2",{3,7}},
    // PB row 4
    {"f1",{4,0}},       {"z",{4,1}},    {"c",{4,2}},   {"b",{4,3}},
    {"m",{4,4}},        {"period",{4,5}},{"right_shift",{4,6}},{"space",{4,7}},
    // PB row 5
    {"f3",{5,0}},       {"s",{5,1}},    {"f",{5,2}},   {"h",{5,3}},
    {"k",{5,4}},        {"colon",{5,5}},{"equal",{5,6}},{"cbm",{5,7}},
    // PB row 6
    {"f5",{6,0}},       {"e",{6,1}},    {"t",{6,2}},   {"u",{6,3}},
    {"o",{6,4}},        {"at",{6,5}},   {"arrow_up",{6,6}},{"q",{6,7}},
    // PB row 7
    {"crsr_down",{7,0}},{"left_shift",{7,1}},{"x",{7,2}},{"v",{7,3}},
    {"n",{7,4}},        {"comma",{7,5}},{"slash",{7,6}},{"run_stop",{7,7}},
};

} // namespace

// ---------------------------------------------------------------------------
// C64 Machine Descriptor Factory
// ---------------------------------------------------------------------------

MachineDescriptor* createMachineC64() {
    auto* desc = new MachineDescriptor();
    desc->machineId   = "c64";
    desc->displayName = "Commodore 64";
    desc->licenseClass = "proprietary"; // ROMs are proprietary

    // -----------------------------------------------------------------------
    // 1. Memory bus — 64 KB flat RAM; I/O and ROM regions overlaid via hooks
    // -----------------------------------------------------------------------
    auto* bus = new FlatMemoryBus("system", 16);
    desc->buses.push_back({"system", bus});

    // -----------------------------------------------------------------------
    // 2. CPU — MOS 6510
    // -----------------------------------------------------------------------
    ICore*    cpu    = CoreRegistry::instance().createCore("6510");
    MOS6510*  cpu6510 = dynamic_cast<MOS6510*>(cpu);
    if (!cpu) {
        std::cerr << "C64: could not find '6510' core." << std::endl;
    }
    if (cpu) {
        cpu->setDataBus(bus);
        cpu->setCodeBus(bus);
        desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 1});
    }

    // -----------------------------------------------------------------------
    // 3. I/O registry
    // -----------------------------------------------------------------------
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    // -----------------------------------------------------------------------
    // 4. Color RAM (1 KB × 4-bit at $D800)
    // -----------------------------------------------------------------------
    uint8_t* colorRam = new uint8_t[1024];
    std::memset(colorRam, 0, 1024);
    desc->roms.push_back(colorRam);  // owned by MachineDescriptor for cleanup

    // -----------------------------------------------------------------------
    // 5. Device instantiation — use DeviceRegistry if the plugin is loaded,
    //    otherwise construct directly (useful in test environments).
    // -----------------------------------------------------------------------

    // C64 PLA — always constructed directly (no standalone plugin instance)
    auto* pla = new C64PLA();

    auto* vic2 = dynamic_cast<VIC2*>(DeviceRegistry::instance().createDevice("6567"));
    if (!vic2) vic2 = new VIC2("VIC-II", 0xD000);
    else { vic2->setName("VIC-II"); vic2->setBaseAddr(0xD000); }

    auto* sid = dynamic_cast<SID6581*>(DeviceRegistry::instance().createDevice("6581"));
    if (!sid) sid = new SID6581("SID", 0xD400);
    else { sid->setName("SID"); sid->setBaseAddr(0xD400); }

    auto* cia1 = dynamic_cast<CIA6526*>(DeviceRegistry::instance().createDevice("6526"));
    if (!cia1) cia1 = new CIA6526("CIA1", 0xDC00);
    else { cia1->setName("CIA1"); cia1->setBaseAddr(0xDC00); }

    auto* cia2 = dynamic_cast<CIA6526*>(DeviceRegistry::instance().createDevice("6526"));
    if (!cia2) cia2 = new CIA6526("CIA2", 0xDD00);
    else { cia2->setName("CIA2"); cia2->setBaseAddr(0xDD00); }

    // -----------------------------------------------------------------------
    // 6. Wire 6510 I/O port banking signals into the PLA
    // -----------------------------------------------------------------------
    if (cpu6510) {
        pla->setSignals(cpu6510->signalLoram(),
                        cpu6510->signalHiram(),
                        cpu6510->signalCharen());
    }

    // -----------------------------------------------------------------------
    // 7. VIC-II DMA bus + initial bank
    //
    // The VIC-II sees a 16 KB window selected by CIA2 Port A bits 0-1
    // (inverted). Power-on default: DDRA=0 → all inputs → pull-ups drive
    // bits 0-1 high → effective output 11 → ~11 & 3 = 00 → bank 0 ($0000).
    // -----------------------------------------------------------------------
    vic2->setDmaBus(bus);
    vic2->setBankBase(0x0000);

    cia2->setPortAWriteCallback([vic2](uint8_t pra, uint8_t ddra) {
        // Effective output on bits 0-1: output-configured bits from pra;
        // input-configured bits float high (pull-up on real hardware).
        uint8_t effective = (pra & ddra) | (~ddra & 0xFF);
        uint8_t bankBits  = (~effective) & 0x03;
        vic2->setBankBase((uint32_t)bankBits * 0x4000);
    });

    // -----------------------------------------------------------------------
    // 8. IRQ / NMI signal lines
    //    - VIC-II raster + CIA1 timer/TOD → CPU IRQ (open-collector, shared)
    //    - CIA2 → CPU NMI (Restore key on real hardware)
    // -----------------------------------------------------------------------
    if (cpu) {
        auto* irqLine = new CpuIrqLine(cpu);
        auto* nmiLine = new CpuNmiLine(cpu);
        vic2->setIrqLine(irqLine);
        cia1->setIrqLine(irqLine);
        cia2->setIrqLine(nmiLine);
    }

    // PAL clock (NTSC = 1022727)
    cia1->setClockHz(985248);
    cia2->setClockHz(985248);

    // -----------------------------------------------------------------------
    // 9. Register handlers in IORegistry.
    // Sort order matters: PLA at $A000 is examined before $D000 devices so it
    // can intercept ROM regions before VIC-II/SID/CIA dispatch.
    // -----------------------------------------------------------------------
    // -----------------------------------------------------------------------
    // 8.5 Keyboard matrix — wired to CIA1 Port A (columns) and Port B (rows).
    //     Registered so resetAll() clears key state on machine reset.
    // -----------------------------------------------------------------------
    auto* kbd = new KbdC64();
    cia1->setPortADevice(kbd->getColumnPort());
    cia1->setPortBDevice(kbd->getRowPort());
    desc->onKey = [kbd](const std::string& keyName, bool down) {
        return kbd->pressKeyByName(keyName, down);
    };

    io->registerHandler(pla);                       // $A000 — sort anchor
    io->registerHandler(vic2);                      // $D000–$D3FF
    io->registerHandler(sid);                       // $D400–$D7FF
    io->registerHandler(new ColorRamHandler(colorRam)); // $D800–$DBFF
    io->registerHandler(cia1);                      // $DC00–$DCFF
    io->registerHandler(cia2);                      // $DD00–$DDFF
    io->registerHandler(kbd);                       // keyboard (null handler, for reset)

    // -----------------------------------------------------------------------
    // 10. Wire bus I/O hooks so every CPU read/write passes through the
    //     IORegistry (PLA + device handlers) before reaching flat RAM.
    // -----------------------------------------------------------------------
    bus->setIoHooks(
        [io, bus](uint32_t addr, uint8_t* val) { return io->dispatchRead (bus, addr, val); },
        [io, bus](uint32_t addr, uint8_t  val) { return io->dispatchWrite(bus, addr, val); }
    );

    // -----------------------------------------------------------------------
    // 11. ROM images
    // -----------------------------------------------------------------------
    uint8_t* basicRom  = new uint8_t[8192]();   // zero-initialised
    uint8_t* kernalRom = new uint8_t[8192]();
    uint8_t* charRom   = new uint8_t[4096]();

    desc->roms.push_back(basicRom);
    desc->roms.push_back(kernalRom);
    desc->roms.push_back(charRom);

    if (!romLoad("roms/c64/basic.bin",  basicRom,  8192))
        std::cerr << "C64: Warning: roms/c64/basic.bin not found." << std::endl;
    if (!romLoad("roms/c64/kernal.bin", kernalRom, 8192))
        std::cerr << "C64: Warning: roms/c64/kernal.bin not found." << std::endl;
    if (!romLoad("roms/c64/char.bin",   charRom,   4096))
        std::cerr << "C64: Warning: roms/c64/char.bin not found." << std::endl;

    // PLA serves ROM data on reads; VIC-II uses char ROM for character DMA.
    pla->setBasicRom (basicRom,  8192);
    pla->setKernalRom(kernalRom, 8192);
    pla->setCharRom  (charRom,   4096);
    vic2->setCharRom (charRom,   4096);
    vic2->setColorRam(colorRam);   // direct 4-bit connection, bypasses CPU banking

    // Add bus overlays so peek8() (debugger/disassembler) sees ROM content.
    // The PLA IO hook intercepts read8() first; when ROM is banked out the
    // PLA returns flat RAM via setRamData() — overlays are never reached then.
    bus->addOverlay(0xA000, 8192, basicRom,  false);  // BASIC ROM
    bus->addOverlay(0xE000, 8192, kernalRom, false);  // KERNAL ROM
    bus->addOverlay(0xD000, 4096, charRom,   false);  // Char ROM

    // Give the PLA a pointer to flat RAM so it can return correct data when
    // a ROM region is banked out (prevents the overlay from serving ROM bytes).
    pla->setRamData(bus->rawData());

    // -----------------------------------------------------------------------
    // 12. Lifecycle callbacks
    // -----------------------------------------------------------------------
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
