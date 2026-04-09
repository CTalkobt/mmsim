#include "test_harness.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/memory_bus.h"
#include "cpu6510.h"
#include "plugin_loader/main/plugin_loader.h"
#include "cli/main/cli_interpreter.h"
#include "plugins/devices/datasette/main/datasette.h"
#include <vector>
#include <string>
#include <iostream>

static void ensureRegistriesReady() {
    PluginLoader::instance().loadFromDir("./lib");
}

TEST_CASE(c64_tape_load) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    
    // Create C64 machine
    cli.processLine("create c64");
    ASSERT(ctx.machine != nullptr);
    auto* desc = ctx.machine;
    auto* bus = static_cast<FlatMemoryBus*>(desc->buses[0].bus);
    
    // Find Datasette device
    std::vector<IOHandler*> handlers;
    desc->ioRegistry->enumerate(handlers);
    Datasette* tape = nullptr;
    for (auto* h : handlers) {
        if (std::string(h->name()) == "Tape") {
            tape = static_cast<Datasette*>(h);
            break;
        }
    }
    ASSERT(tape != nullptr);

    // Mount the .tap file
    bool mounted = tape->mountTape("tests/resources/China_Miner_Demonstration_Cassette.tap");
    ASSERT(mounted);

    // Start motor: set bit 5 of 6510 port to 0.
    // We must write via the CPU's data bus so the PortBus proxy intercepts it.
    IBus* cpuBus = desc->cpus[0].cpu->getDataBus();
    cpuBus->write8(0x0000, 0x2f); // bits 0-3, 5 are output
    cpuBus->write8(0x0001, 0x1f); // bit 5 = 0 (motor on)

    // Run cycles to see if CIA1 FLAG (bit 4 of ICR) triggers.
    bool flagFired = false;
    for (int i = 0; i < 10000000; ++i) {
        desc->schedulerStep(*desc);
        
        uint8_t icr = 0;
        if (desc->ioRegistry->dispatchRead(bus, 0xDC0D, &icr)) {
            if (icr & 0x10) { // FLAG bit
                flagFired = true;
                break;
            }
        }
    }

    ASSERT(flagFired);
}
