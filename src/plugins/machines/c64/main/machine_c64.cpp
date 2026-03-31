#include "libcore/main/machine_desc.h"
#include "mmemu_plugin_api.h"
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

const struct SimPluginHostAPI* g_c64Host = nullptr;

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
        if (m_cpu) m_cpu->setIrqLine(level);
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
        if (m_cpu) m_cpu->setNmiLine(level);
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
// ---------------------------------------------------------------------------

class KbdC64 : public IOHandler {
public:
    KbdC64() : m_colPort(this), m_rowPort(this) { clearKeys(); }
    ~KbdC64() override = default;

    const char* name()     const override { return "C64Keyboard"; }
    uint32_t    baseAddr() const override { return 0; }
    uint32_t    addrMask() const override { return 0; }
    bool ioRead (IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
    void reset() override { clearKeys(); }
    void tick(uint64_t) override {}

    void clearKeys() {
        std::memset(m_matrix, 0xFF, sizeof(m_matrix));
        updateMatrix();
    }

    bool pressKeyByName(const std::string& keyName, bool down) {
        if (g_c64Host && g_c64Host->log) {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "KbdC64: pressKeyByName '%s' down=%d", keyName.c_str(), (int)down);
            g_c64Host->log(SIM_LOG_DEBUG, buf);
        }
        std::string lower = keyName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "exclaim")   return pressCombo({"left_shift", "1"}, down);
        if (lower == "dquote")    return pressCombo({"left_shift", "2"}, down);
        if (lower == "hash")      return pressCombo({"left_shift", "3"}, down);
        if (lower == "dollar")    return pressCombo({"left_shift", "4"}, down);
        if (lower == "percent")   return pressCombo({"left_shift", "5"}, down);
        if (lower == "ampersand") return pressCombo({"left_shift", "6"}, down);
        if (lower == "squote")    return pressCombo({"left_shift", "7"}, down); 
        if (lower == "lparen")    return pressCombo({"left_shift", "8"}, down);
        if (lower == "rparen")    return pressCombo({"left_shift", "9"}, down);
        if (lower == "question")  return pressCombo({"left_shift", "slash"}, down);
        if (lower == "less")      return pressCombo({"left_shift", "comma"}, down);
        if (lower == "greater")   return pressCombo({"left_shift", "period"}, down);
        if (lower == "bracket_left")  return pressCombo({"left_shift", "colon"}, down);
        if (lower == "bracket_right") return pressCombo({"left_shift", "semicolon"}, down);

        if (lower == "uparrow")   lower = "arrow_up";
        if (lower == "leftarrow") lower = "arrow_left";
        if (lower == "right")     lower = "crsr_right";
        if (lower == "down")      lower = "crsr_down";
        if (lower == "up")        return pressCombo({"left_shift", "crsr_down"}, down);
        if (lower == "left")      return pressCombo({"left_shift", "crsr_right"}, down);

        auto it = s_keyMap.find(lower);
        if (it != s_keyMap.end()) {
            int pa = it->second.second;
            int pb = it->second.first;
            if (down) m_matrix[pa] &= ~(1 << pb);
            else      m_matrix[pa] |=  (1 << pb);
            updateMatrix();
            return true;
        }
        return false;
    }

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
    bool pressCombo(const std::vector<std::string>& keys, bool down) {
        bool ok = true;
        for (const auto& k : keys) {
            if (!down && (k == "left_shift" || k == "right_shift" || k == "ctrl" || k == "cbm")) {
                continue;
            }
            auto it = s_keyMap.find(k);
            if (it != s_keyMap.end()) {
                int pa = it->second.second;
                int pb = it->second.first;
                if (down) m_matrix[pa] &= ~(1 << pb);
                else      m_matrix[pa] |=  (1 << pb);
            } else {
                ok = false;
            }
        }
        updateMatrix();
        return ok;
    }

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

/**
 * VIC-II DMA Bus — bypasses CPU banking (PLA) and I/O.
 */
class C64DmaBus : public IBus {
public:
    explicit C64DmaBus(FlatMemoryBus* base) : m_base(base) {}
    const BusConfig& config() const override { return m_base->config(); }
    const char*      name()   const override { return "C64DmaBus"; }
    uint8_t read8(uint32_t addr)  override { return m_base->read8Raw(addr); }
    void    write8(uint32_t, uint8_t) override {}
    uint8_t peek8(uint32_t addr)  override { return m_base->peek8Raw(addr); }
    size_t stateSize() const override { return 0; }
    void   saveState(uint8_t*) const override {}
    void   loadState(const uint8_t*) override {}
    int    writeCount() const override { return 0; }
    void   getWrites(uint32_t*, uint8_t*, uint8_t*, int) const override {}
    void   clearWriteLog() override {}
private:
    FlatMemoryBus* m_base;
};

std::map<std::string, std::pair<int,int>> KbdC64::s_keyMap = {
    {"delete",{0,0}},   {"3",{0,1}},    {"5",{0,2}},   {"7",{0,3}},
    {"9",{0,4}},        {"plus",{0,5}}, {"pound",{0,6}},{"1",{0,7}},
    {"return",{1,0}},   {"w",{1,1}},    {"r",{1,2}},   {"y",{1,3}},
    {"i",{1,4}},        {"p",{1,5}},    {"asterisk",{1,6}},{"arrow_left",{1,7}},
    {"crsr_right",{2,0}},{"a",{2,1}},  {"d",{2,2}},   {"g",{2,3}},
    {"j",{2,4}},        {"l",{2,5}},    {"semicolon",{2,6}},{"ctrl",{2,7}},
    {"f7",{3,0}},       {"4",{3,1}},    {"6",{3,2}},   {"8",{3,3}},
    {"0",{3,4}},        {"minus",{3,5}},{"home",{3,6}}, {"2",{3,7}},
    {"f1",{4,0}},       {"z",{4,1}},    {"c",{4,2}},   {"b",{4,3}},
    {"m",{4,4}},        {"period",{4,5}},{"right_shift",{4,6}},{"space",{4,7}},
    {"f3",{5,0}},       {"s",{5,1}},    {"f",{5,2}},   {"h",{5,3}},
    {"k",{5,4}},        {"colon",{5,5}},{"equal",{5,6}},{"cbm",{5,7}},
    {"f5",{6,0}},       {"e",{6,1}},    {"t",{6,2}},   {"u",{6,3}},
    {"o",{6,4}},        {"at",{6,5}},   {"arrow_up",{6,6}},{"q",{6,7}},
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
    desc->licenseClass = "proprietary"; 

    auto* bus = new FlatMemoryBus("system", 16);
    desc->buses.push_back({"system", bus});

    ICore*    cpu    = nullptr;
    if (g_c64Host && g_c64Host->createCore) {
        cpu = g_c64Host->createCore("6510");
    } else {
        cpu = CoreRegistry::instance().createCore("6510");
    }

    MOS6510*  cpu6510 = dynamic_cast<MOS6510*>(cpu);
    if (cpu) {
        cpu->setDataBus(bus);
        cpu->setCodeBus(bus);
        IBus* dataBus = bus;
        if (cpu6510 && cpu6510->getPortBus()) {
            dataBus = cpu6510->getPortBus();
        }
        desc->cpus.push_back({"main", cpu, dataBus, dataBus, nullptr, true, 1});
    }

    auto* io = new IORegistry();
    desc->ioRegistry = io;

    uint8_t* colorRam = new uint8_t[1024];
    std::memset(colorRam, 0, 1024);
    desc->roms.push_back(colorRam);

    auto* pla = new C64PLA();
    VIC2* vic2 = nullptr;
    SID6581* sid = nullptr;
    CIA6526 *cia1 = nullptr, *cia2 = nullptr;

    if (g_c64Host && g_c64Host->createDevice) {
        vic2 = dynamic_cast<VIC2*>(g_c64Host->createDevice("6567"));
        sid = dynamic_cast<SID6581*>(g_c64Host->createDevice("6581"));
        cia1 = dynamic_cast<CIA6526*>(g_c64Host->createDevice("6526"));
        cia2 = dynamic_cast<CIA6526*>(g_c64Host->createDevice("6526"));
    } else {
        vic2 = dynamic_cast<VIC2*>(DeviceRegistry::instance().createDevice("6567"));
        sid = dynamic_cast<SID6581*>(DeviceRegistry::instance().createDevice("6581"));
        cia1 = dynamic_cast<CIA6526*>(DeviceRegistry::instance().createDevice("6526"));
        cia2 = dynamic_cast<CIA6526*>(DeviceRegistry::instance().createDevice("6526"));
    }

    if (!vic2) vic2 = new VIC2("VIC-II", 0xD000);
    else { vic2->setName("VIC-II"); vic2->setBaseAddr(0xD000); }
    if (!sid) sid = new SID6581("SID", 0xD400);
    else { sid->setName("SID"); sid->setBaseAddr(0xD400); }
    if (!cia1) cia1 = new CIA6526("CIA1", 0xDC00);
    else { cia1->setName("CIA1"); cia1->setBaseAddr(0xDC00); }
    if (!cia2) cia2 = new CIA6526("CIA2", 0xDD00);
    else { cia2->setName("CIA2"); cia2->setBaseAddr(0xDD00); }

    if (cpu6510) {
        pla->setSignals(cpu6510->signalLoram(),
                        cpu6510->signalHiram(),
                        cpu6510->signalCharen());
    }

    auto* dmaBus = new C64DmaBus(bus);
    desc->deleters.push_back([dmaBus]() { delete dmaBus; });
    vic2->setDmaBus(dmaBus);
    vic2->setBankBase(0x0000);

    cia2->setPortAWriteCallback([vic2](uint8_t pra, uint8_t ddra) {
        uint8_t effective = (pra & ddra) | (~ddra & 0xFF);
        uint8_t bankBits  = (~effective) & 0x03;
        vic2->setBankBase((uint32_t)bankBits * 0x4000);
    });

    if (cpu) {
        auto* irqLine = new CpuIrqLine(cpu);
        auto* nmiLine = new CpuNmiLine(cpu);
        vic2->setIrqLine(irqLine);
        cia1->setIrqLine(irqLine);
        cia2->setIrqLine(nmiLine);
        desc->deleters.push_back([irqLine]() { delete irqLine; });
        desc->deleters.push_back([nmiLine]() { delete nmiLine; });
    }

    cia1->setClockHz(985248);
    cia2->setClockHz(985248);

    auto* kbd = new KbdC64();
    cia1->setPortADevice(kbd->getColumnPort());
    cia1->setPortBDevice(kbd->getRowPort());
    desc->onKey = [kbd](const std::string& keyName, bool down) {
        return kbd->pressKeyByName(keyName, down);
    };

    io->registerOwnedHandler(pla);
    io->registerOwnedHandler(vic2);
    io->registerOwnedHandler(sid);
    io->registerOwnedHandler(new ColorRamHandler(colorRam));
    io->registerOwnedHandler(cia1);
    io->registerOwnedHandler(cia2);
    io->registerOwnedHandler(kbd);

    bus->setIoHooks(
        [io](IBus* b, uint32_t addr, uint8_t* val) { return io->dispatchRead (b, addr, val); },
        [io](IBus* b, uint32_t addr, uint8_t  val) { return io->dispatchWrite(b, addr, val); }
    );

    uint8_t* basicRom  = new uint8_t[8192]();
    uint8_t* kernalRom = new uint8_t[8192]();
    uint8_t* charRom   = new uint8_t[4096]();
    desc->roms.push_back(basicRom);
    desc->roms.push_back(kernalRom);
    desc->roms.push_back(charRom);

    romLoad("roms/c64/basic.bin",  basicRom,  8192);
    romLoad("roms/c64/kernal.bin", kernalRom, 8192);
    romLoad("roms/c64/char.bin",   charRom,   4096);

    pla->setBasicRom (basicRom,  8192);
    pla->setKernalRom(kernalRom, 8192);
    pla->setCharRom  (charRom,   4096);
    vic2->setCharRom (charRom,   4096);
    vic2->setColorRam(colorRam);
    pla->setRamData(bus->rawData());

    desc->onReset = [](MachineDescriptor& d) {
        if (d.ioRegistry) d.ioRegistry->resetAll();
        for (auto& slot : d.cpus) if (slot.cpu) slot.cpu->reset();
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
