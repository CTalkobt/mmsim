#include "json_machine_loader.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/rom_loader.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/io_registry.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/isignal_line.h"
#include <fstream>
#include <sstream>
#include <map>
#include <string>

// ---------------------------------------------------------------------------
// Local signal-line helpers (CpuIrqLine, CpuNmiLine)
// These forward level-changes to the CPU interrupt pins.
// Phase 5 will move them to a shared header.
// ---------------------------------------------------------------------------

namespace {

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

} // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static uint32_t hexStrToU32(const std::string& s) {
    return static_cast<uint32_t>(std::stoul(s, nullptr, 0));
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

int JsonMachineLoader::loadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return 0;
    std::ostringstream ss;
    ss << f.rdbuf();
    return loadString(ss.str());
}

int JsonMachineLoader::loadString(const std::string& json) {
    try {
        auto doc = nlohmann::json::parse(json);
        return registerAll(doc);
    } catch (const nlohmann::json::exception&) {
        return 0;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int JsonMachineLoader::registerAll(const nlohmann::json& doc) {
    const nlohmann::json* machines = nullptr;
    nlohmann::json singleArr;

    if (doc.is_array()) {
        machines = &doc;
    } else if (doc.is_object()) {
        if (doc.contains("machines") && doc["machines"].is_array()) {
            machines = &doc["machines"];
        } else {
            singleArr = nlohmann::json::array({ doc });
            machines  = &singleArr;
        }
    } else {
        return 0;
    }

    int count = 0;
    for (const auto& spec : *machines) {
        if (!spec.contains("id") || !spec["id"].is_string()) continue;
        std::string id = spec["id"].get<std::string>();

        nlohmann::json specCopy = spec;
        MachineRegistry::instance().registerMachine(id,
            [this, specCopy]() -> MachineDescriptor* {
                return buildFromSpec(specCopy);
            });
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// buildFromSpec — Phase 2 implementation
// Steps 3–9: bus, cpu, roms, devices, io hook, signals, scheduler, reset.
// ---------------------------------------------------------------------------

MachineDescriptor* JsonMachineLoader::buildFromSpec(const nlohmann::json& spec) {
    auto* desc = new MachineDescriptor();

    desc->machineId    = spec.value("id",          "");
    desc->displayName  = spec.value("displayName",  "");
    desc->licenseClass = spec.value("licenseClass", "");

    // -----------------------------------------------------------------------
    // Step 3 — Bus
    // -----------------------------------------------------------------------
    FlatMemoryBus* bus = nullptr;
    std::map<std::string, IBus*> busPtrs;

    if (spec.contains("bus")) {
        const auto& busSpec = spec["bus"];
        std::string busName = busSpec.value("name",     "system");
        int         addrBits = busSpec.value("addrBits", 16);
        bus = new FlatMemoryBus(busName, addrBits);
        desc->buses.push_back({busName, bus});
        busPtrs[busName] = bus;
    }

    // -----------------------------------------------------------------------
    // Step 3 — CPU
    // -----------------------------------------------------------------------
    ICore* cpu = nullptr;

    if (spec.contains("cpu")) {
        const auto& cpuSpec = spec["cpu"];
        std::string cpuType = cpuSpec.value("type", "");
        cpu = CoreRegistry::instance().createCore(cpuType);

        if (cpu) {
            std::string dataBusName = cpuSpec.value("dataBus", "system");
            std::string codeBusName = cpuSpec.value("codeBus", "system");

            IBus* dataBus = busPtrs.count(dataBusName) ? busPtrs[dataBusName] : bus;
            IBus* codeBus = busPtrs.count(codeBusName) ? busPtrs[codeBusName] : bus;

            // portBusAsData: true → Phase 3 (MOS6510 port bus wiring for C64)

            cpu->setDataBus(dataBus);
            cpu->setCodeBus(codeBus);
            desc->cpus.push_back({"main", cpu, dataBus, codeBus, nullptr, true, 1});
        }
    }

    // -----------------------------------------------------------------------
    // Step 6 — IORegistry (created before devices so handlers can register)
    // -----------------------------------------------------------------------
    auto* io = new IORegistry();
    desc->ioRegistry = io;

    // -----------------------------------------------------------------------
    // Step 4 — ROM allocation and loading
    // -----------------------------------------------------------------------
    std::map<std::string, uint8_t*> romPtrs;

    if (spec.contains("roms")) {
        for (const auto& romSpec : spec["roms"]) {
            std::string label    = romSpec.value("label",    "");
            std::string path     = romSpec.value("path",     "");
            int         size     = romSpec.value("size",     0);
            bool        writable = romSpec.value("writable", false);

            if (size <= 0) continue;

            auto* ptr = new uint8_t[size]();
            desc->roms.push_back(ptr);
            if (!label.empty()) romPtrs[label] = ptr;

            if (!path.empty())
                romLoad(path.c_str(), ptr, (size_t)size);

            if (bus && romSpec.contains("overlayAddr") &&
                !romSpec["overlayAddr"].is_null()) {
                uint32_t addr = hexStrToU32(
                    romSpec["overlayAddr"].get<std::string>());
                bus->addOverlay(addr, (uint32_t)size, ptr, writable);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Step 5 — Device creation
    // -----------------------------------------------------------------------
    std::map<std::string, IOHandler*> devPtrs;

    if (spec.contains("devices")) {
        for (const auto& devSpec : spec["devices"]) {
            if (devSpec.value("_same", false)) continue;

            std::string type = devSpec.value("type", "");
            if (type.size() >= 7 && type.substr(0, 7) == "inline_") continue;

            std::string devName = devSpec.value("name", "");

            IOHandler* dev = DeviceRegistry::instance().createDevice(type);
            if (!dev) continue;

            dev->setName(devName);

            if (devSpec.contains("baseAddr") && !devSpec["baseAddr"].is_null()) {
                dev->setBaseAddr(
                    hexStrToU32(devSpec["baseAddr"].get<std::string>()));
            }

            if (devSpec.contains("clockHz") && !devSpec["clockHz"].is_null()) {
                dev->setClockHz(devSpec["clockHz"].get<uint32_t>());
            }

            devPtrs[devName] = dev;
            io->registerOwnedHandler(dev);
        }
    }

    // -----------------------------------------------------------------------
    // Step 6 — Bus I/O hook
    // -----------------------------------------------------------------------
    if (bus) {
        bus->setIoHooks(
            [io](IBus* b, uint32_t a, uint8_t* v) { return io->dispatchRead (b, a, v); },
            [io](IBus* b, uint32_t a, uint8_t  v) { return io->dispatchWrite(b, a, v); });
    }

    // -----------------------------------------------------------------------
    // Step 7 — Signal lines (simple IRQ/NMI; shared→Phase 3)
    // -----------------------------------------------------------------------
    if (spec.contains("signals")) {
        for (const auto& sigSpec : spec["signals"]) {
            std::string sigName = sigSpec.value("name", "");
            std::string sigType = sigSpec.value("type", "simple");

            if (sigType != "simple") continue;  // "shared" → Phase 3

            ISignalLine* line = nullptr;
            if (sigName == "irq" && cpu)
                line = new CpuIrqLine(cpu);
            else if (sigName == "nmi" && cpu)
                line = new CpuNmiLine(cpu);

            if (!line) continue;

            desc->deleters.push_back([line]() { delete line; });

            if (sigSpec.contains("sources")) {
                for (const auto& srcEntry : sigSpec["sources"]) {
                    std::string srcName = srcEntry.get<std::string>();
                    if (!devPtrs.count(srcName)) continue;
                    if (sigName == "irq")
                        devPtrs[srcName]->setIrqLine(line);
                    else
                        devPtrs[srcName]->setNmiLine(line);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Step 8 — Scheduler
    // -----------------------------------------------------------------------
    bool hasFrameAccum = false;
    if (spec.contains("scheduler")) {
        const auto& sched = spec["scheduler"];
        hasFrameAccum = sched.contains("frameAccumulator") &&
                        !sched["frameAccumulator"].is_null();
    }

    if (!hasFrameAccum) {
        desc->schedulerStep = [](MachineDescriptor& d) {
            int total = 0;
            for (auto& s : d.cpus) {
                if (s.active && s.cpu) {
                    total += s.cpu->step();
                    if (d.ioRegistry) d.ioRegistry->tickAll(total);
                }
            }
            return total;
        };
    }
    // hasFrameAccum scheduler → Phase 3 (Step 15)

    // -----------------------------------------------------------------------
    // Step 9 — onReset
    // -----------------------------------------------------------------------
    desc->onReset = [](MachineDescriptor& d) {
        if (d.ioRegistry) d.ioRegistry->resetAll();
        for (auto& slot : d.cpus)
            if (slot.cpu) slot.cpu->reset();
    };

    return desc;
}
