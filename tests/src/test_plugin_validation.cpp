#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libdevices/main/device_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include "plugin_loader/main/plugin_loader.h"
#include <iostream>
#include <vector>
#include <string>

static void loadAllPlugins() {
    PluginLoader::instance().loadFromDir("./lib");
}

TEST_CASE(validate_all_plugins) {
    loadAllPlugins();

    std::cout << "Validating Cores..." << std::endl;
    std::vector<CoreRegistry::CoreInfo> cores;
    CoreRegistry::instance().enumerate(cores);
    for (const auto& info : cores) {
        ICore* core = CoreRegistry::instance().createCore(info.name);
        ASSERT(core != nullptr);
        ASSERT(core->isaName() != nullptr);
        std::cout << "  Core: " << info.name << " (ISA: " << core->isaName() << ")" << std::endl;
        delete core;
    }

    std::cout << "Validating Toolchains..." << std::endl;
    // We can't easily enumerate toolchains because enumerate is private/missing or just returns ISA names
    // Let's assume we can at least validate those used by cores
    for (const auto& info : cores) {
        IDisassembler* dis = ToolchainRegistry::instance().createDisassembler(info.name);
        if (dis) {
            ASSERT(dis->isaName() != nullptr);
            std::cout << "  Disassembler for " << info.name << ": " << dis->isaName() << std::endl;
            delete dis;
        }
    }

    std::cout << "Validating Devices..." << std::endl;
    std::vector<std::string> deviceTypes;
    DeviceRegistry::instance().enumerate(deviceTypes);
    for (const auto& type : deviceTypes) {
        IOHandler* device = DeviceRegistry::instance().createDevice(type);
        ASSERT(device != nullptr);
        const char* name = device->name();
        if (name == nullptr) {
            std::cerr << "CRITICAL: Device type '" << type << "' returned NULL for name()!" << std::endl;
        }
        ASSERT(name != nullptr);
        std::cout << "  Device: " << type << " (Instance name: " << name << ")" << std::endl;
        delete device;
    }

    std::cout << "Validating Machines..." << std::endl;
    std::vector<std::string> machineIds;
    MachineRegistry::instance().enumerate(machineIds);
    for (const auto& id : machineIds) {
        MachineDescriptor* machine = MachineRegistry::instance().createMachine(id);
        ASSERT(machine != nullptr);
        ASSERT(!machine->machineId.empty());
        std::cout << "  Machine: " << id << " (Descriptor ID: " << machine->machineId << ")" << std::endl;
        
        // Also check all devices in the machine
        if (machine->ioRegistry) {
            std::vector<IOHandler*> handlers;
            machine->ioRegistry->enumerate(handlers);
            for (auto* h : handlers) {
                const char* hname = h->name();
                if (hname == nullptr) {
                    std::cerr << "CRITICAL: Machine '" << id << "' has device with NULL name!" << std::endl;
                }
                ASSERT(hname != nullptr);
            }
        }
        
        delete machine;
    }
}
