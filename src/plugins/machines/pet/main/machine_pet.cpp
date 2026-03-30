#include "libcore/main/machine_desc.h"
#include "libcore/main/rom_loader.h"
#include "libcore/main/core_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/isignal_line.h"
#include "libmem/main/memory_bus.h"
#include "plugins/devices/pia6520/main/pia6520.h"
#include "plugins/devices/via6522/main/via6522.h"
#include "plugins/devices/crtc6545/main/crtc6545.h"
#include "plugins/devices/pet_video/main/pet_video.h"
#include "plugins/devices/keyboard/main/keyboard_matrix_pet.h"
#include "libdevices/main/ieee488.h"
#include "machine_pet.h"
#include "mmemu_plugin_api.h"
#include <iostream>
#include <map>
#include <vector>

extern const SimPluginHostAPI* g_petHost;

namespace {

/**
 * Manages a shared IRQ line where multiple devices can assert interrupts.
 * Implements Wired-OR logic (if ANY input is true, the CPU IRQ pin is true).
 */
class SharedIrqManager {
public:
    explicit SharedIrqManager(ICore* cpu) : m_cpu(cpu) {}

    class DeviceLine : public ISignalLine {
    public:
        DeviceLine(SharedIrqManager* parent, uint32_t bit) : m_parent(parent), m_bit(bit) {}
        bool get() const override { return m_level; }
        void pulse() override { m_parent->m_cpu->triggerIrq(); }
        void set(bool level) override {
            m_level = level;
            if (level) m_parent->m_state |= m_bit;
            else       m_parent->m_state &= ~m_bit;
            
            m_parent->m_cpu->setIrqLine(m_parent->m_state != 0);
        }
    private:
        SharedIrqManager* m_parent;
        uint32_t          m_bit;
        bool              m_level = false;
    };

    ISignalLine* createLine() {
        uint32_t bit = 1u << m_lines.size();
        auto* line = new DeviceLine(this, bit);
        m_lines.push_back(line);
        return line;
    }

private:
    ICore* m_cpu;
    uint32_t m_state = 0;
    std::vector<ISignalLine*> m_lines;
};

// A simple latching signal line for vertical sync
class SimpleSignalLine : public ISignalLine {
public:
    bool get() const override { return m_level; }
    void set(bool level) override { m_level = level; }
    void pulse() override { m_level = !m_level; m_level = !m_level; }
private:
    bool m_level = true;
};

// Adapter to read keyboard columns
class KbdColumnReader : public IPortDevice {
public:
    KbdColumnReader(PetKeyboardMatrix* kbd) : m_kbd(kbd) {}
    uint8_t readPort() override { return m_kbd->readPort(); }
    void writePort(uint8_t) override {}
    void setDdr(uint8_t) override {}
private:
    PetKeyboardMatrix* m_kbd;
};

// Adapter to handle keyboard row selection and diagnostic pin
class KbdRowSelector : public IPortDevice {
public:
    KbdRowSelector(PetKeyboardMatrix* kbd) : m_kbd(kbd) {}
    uint8_t readPort() override {
        // Return 1 on bit 7 (Diagnostic pin)
        return (m_lastVal & 0x7F) | 0x80;
    }
    void writePort(uint8_t val) override {
        m_lastVal = val;
        m_kbd->writePort(val);
    }
    void setDdr(uint8_t) override {}
private:
    PetKeyboardMatrix* m_kbd;
    uint8_t m_lastVal = 0xFF;
};

} // namespace

struct PetMachine {
    Model model;
    FlatMemoryBus* bus = nullptr;
    ICore* cpu = nullptr;
    SimpleIEEE488Bus* ieee = nullptr;
    PIA6520* pia1 = nullptr;
    PIA6520* pia2 = nullptr;
    VIA6522* via = nullptr;
    CRTC6545* crtc = nullptr;
    PetVideo* video = nullptr;
    PetKeyboardMatrix* kbd = nullptr;
    KbdColumnReader* kbdReader = nullptr;
    KbdRowSelector* kbdSelector = nullptr;

    SimpleSignalLine* videoLine = nullptr;
    SharedIrqManager* irqManager = nullptr;

    uint8_t* basicRom = nullptr;
    uint8_t* kernalRom = nullptr;
    uint8_t* editorRom = nullptr;
    uint8_t* charRom = nullptr;

    uint64_t frameAccum = 0;
    static constexpr uint64_t CYCLES_PER_FRAME = 20000; 
};

static std::map<MachineDescriptor*, PetMachine*> s_petInstances;

static void petReset(MachineDescriptor& desc) {
    auto* pm = s_petInstances[&desc];
    if (!pm) return;
    
    if (desc.ioRegistry) desc.ioRegistry->resetAll();
    pm->ieee->reset();
    pm->cpu->reset();
}

MachineDescriptor* createPetMachine(Model model) {
    auto* pm = new PetMachine();
    pm->model = model;
    
    auto* desc = new MachineDescriptor();
    s_petInstances[desc] = pm;
    desc->machineId = (model == Model::PET_2001) ? "pet2001" : 
                      (model == Model::PET_8032) ? "pet8032" : "pet4032";
    desc->displayName = (model == Model::PET_2001) ? "Commodore PET 2001" : 
                        (model == Model::PET_8032) ? "Commodore PET 8032" : "Commodore PET 4032";

    pm->bus = new FlatMemoryBus("system", 16);
    pm->cpu = CoreRegistry::instance().createCore("6502");
    desc->ioRegistry = new IORegistry();
    pm->ieee = new SimpleIEEE488Bus();

    // IRQ Manager: Orchestrates shared IRQ signals from multiple chips
    pm->irqManager = new SharedIrqManager(pm->cpu);

    // Video retrace line (Vertical Sync)
    pm->videoLine = new SimpleSignalLine();

    // I/O Devices
    pm->pia1 = new PIA6520("PIA1", 0xE810);
    if (g_petHost && g_petHost->getLogger && g_petHost->logNamed)
        pm->pia1->setLogger(g_petHost->getLogger("pia1"), g_petHost->logNamed);
    pm->pia1->setIrqALine(pm->irqManager->createLine());
    pm->pia1->setIrqBLine(pm->irqManager->createLine());
    pm->pia1->setCA1Line(pm->videoLine);
    pm->pia1->setCB1Line(pm->videoLine);

    pm->pia2 = new PIA6520("PIA2", 0xE820);
    if (g_petHost && g_petHost->getLogger && g_petHost->logNamed)
        pm->pia2->setLogger(g_petHost->getLogger("pia2"), g_petHost->logNamed);
    pm->pia2->setIrqALine(pm->irqManager->createLine());
    pm->pia2->setIrqBLine(pm->irqManager->createLine());

    pm->via = new VIA6522("VIA", 0xE840);
    pm->via->setIrqLine(pm->irqManager->createLine());
    pm->via->setCB1Line(pm->videoLine); 
    
    PetVideo::Model vModel = (model == Model::PET_2001) ? PetVideo::Model::PET_2001 : 
                             (model == Model::PET_8032) ? PetVideo::Model::PET_8000 : PetVideo::Model::PET_4000;
    pm->video = new PetVideo(vModel);

    PetKeyboardMatrix::Layout kLayout = (model == Model::PET_8032) ? 
                                        PetKeyboardMatrix::Layout::BUSINESS : PetKeyboardMatrix::Layout::GRAPHICS;
    pm->kbd = new PetKeyboardMatrix(kLayout);
    if (g_petHost && g_petHost->getLogger && g_petHost->logNamed)
        pm->kbd->setLogger(g_petHost->getLogger("pet.keyboard"), g_petHost->logNamed);
    pm->kbdReader = new KbdColumnReader(pm->kbd);
    pm->kbdSelector = new KbdRowSelector(pm->kbd);

    // PIA1 Wiring
    pm->pia1->setPortADevice(pm->kbdReader);
    pm->pia1->setPortAWriteCallback([pm](uint8_t val) { pm->ieee->setData(~val); });
    pm->pia1->setPortBDevice(pm->kbdSelector);

    // PIA2 Wiring: IEEE Control Signals
    pm->pia2->setPortAWriteCallback([pm](uint8_t val) {
        pm->ieee->setSignal(SimpleIEEE488Bus::NDAC, (val & 0x01) == 0);
        pm->ieee->setSignal(SimpleIEEE488Bus::NRFD, (val & 0x02) == 0);
        pm->ieee->setSignal(SimpleIEEE488Bus::ATN,  (val & 0x04) == 0);
    });
    pm->pia2->setPortBWriteCallback([pm](uint8_t val) {
        pm->ieee->setSignal(SimpleIEEE488Bus::DAV, (val & 0x01) == 0);
        pm->ieee->setSignal(SimpleIEEE488Bus::EOI, (val & 0x02) == 0);
    });

    desc->ioRegistry->registerHandler(pm->pia1);
    desc->ioRegistry->registerHandler(pm->pia2);
    desc->ioRegistry->registerHandler(pm->via);
    desc->ioRegistry->registerHandler(pm->video);

    if (model != Model::PET_2001) {
        pm->crtc = new CRTC6545(); 
        desc->ioRegistry->registerHandler(pm->crtc);
        pm->video->setCRTC(pm->crtc);
    }

    // ROM Loading
    bool isBasic2 = (model == Model::PET_2001);
    std::string modelName = desc->machineId;
    size_t basicSize = isBasic2 ? 8192 : 12288;
    
    pm->basicRom = new uint8_t[basicSize]();
    pm->kernalRom = new uint8_t[4096]();
    pm->editorRom = new uint8_t[2048]();
    pm->charRom = new uint8_t[2048]();

    desc->roms.push_back(pm->basicRom);
    desc->roms.push_back(pm->kernalRom);
    desc->roms.push_back(pm->editorRom);
    desc->roms.push_back(pm->charRom);

    std::string basicPath = "roms/" + modelName + "/basic.bin";
    if (romLoad(basicPath.c_str(), pm->basicRom, basicSize)) {
        uint32_t basicAddr = isBasic2 ? 0xC000 : 0xB000;
        pm->bus->addRomOverlay(basicAddr, (uint32_t)basicSize, pm->basicRom);
        desc->overlays.push_back({basicPath, basicAddr, true, true});
    }
    
    std::string kernalPath = "roms/" + modelName + "/kernal.bin";
    if (romLoad(kernalPath.c_str(), pm->kernalRom, 4096)) {
        pm->bus->addRomOverlay(0xF000, 4096, pm->kernalRom);
        desc->overlays.push_back({kernalPath, 0xF000, true, true});
    }
    
    std::string editorPath = "roms/" + modelName + "/editor.bin";
    if (romLoad(editorPath.c_str(), pm->editorRom, 2048)) {
        pm->bus->addRomOverlay(0xE000, 2048, pm->editorRom);
        desc->overlays.push_back({editorPath, 0xE000, true, true});
    }
    
    std::string charPath = "roms/" + modelName + "/char.bin";
    if (romLoad(charPath.c_str(), pm->charRom, 2048)) {
        pm->video->setCharRom(pm->charRom, 2048);
        desc->overlays.push_back({charPath, 0x0000, false, true});
    }

    pm->bus->setIoHooks(
        [desc, pm](IBus* b, uint32_t addr, uint8_t* val) {
            if (addr >= 0xE800 && addr <= 0xE80F) {
                return pm->pia1->ioRead(b, 0xE810 | (addr & 0x03), val);
            }
            return desc->ioRegistry->dispatchRead(b, addr, val);
        },
        [desc, pm](IBus* b, uint32_t addr, uint8_t val) {
            if (addr >= 0xE800 && addr <= 0xE80F) {
                return pm->pia1->ioWrite(b, 0xE810 | (addr & 0x03), val);
            }
            return desc->ioRegistry->dispatchWrite(b, addr, val);
        }
    );

    pm->cpu->setDataBus(pm->bus);
    pm->cpu->setCodeBus(pm->bus);
    
    desc->cpus.push_back({"main", pm->cpu, pm->bus, pm->bus, nullptr, true, 1});
    desc->buses.push_back({"system", pm->bus});
    desc->onReset = petReset;
    desc->onKey = [pm](const std::string& name, bool down) {
        if (down) pm->kbd->keyDown(name);
        else pm->kbd->keyUp(name);
        return true;
    };

    desc->schedulerStep = [pm](MachineDescriptor& d) {
        int totalCycles = 0;
        for (auto& slot : d.cpus) {
            if (slot.active && slot.cpu) {
                int cycles = slot.cpu->step();
                totalCycles += cycles;
                if (d.ioRegistry) d.ioRegistry->tickAll(cycles);
            }
        }
        pm->frameAccum += (uint64_t)totalCycles;
        if (pm->frameAccum >= PetMachine::CYCLES_PER_FRAME) {
            pm->frameAccum -= PetMachine::CYCLES_PER_FRAME;
            pm->videoLine->set(false); 
            if (d.ioRegistry) d.ioRegistry->tickAll(0);
            pm->videoLine->set(true);  
            if (d.ioRegistry) d.ioRegistry->tickAll(0);
        }
        return totalCycles;
    };

    return desc;
}

MachineDescriptor* createPet2001() { return createPetMachine(Model::PET_2001); }
MachineDescriptor* createPet4032() { return createPetMachine(Model::PET_4032); }
MachineDescriptor* createPet8032() { return createPetMachine(Model::PET_8032); }
