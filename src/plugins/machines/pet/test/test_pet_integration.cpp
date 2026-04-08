#include "test_harness.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/core_registry.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/device_registry.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/iport_device.h"
#include "libmem/main/memory_bus.h"
#include "libdevices/main/ieee488.h"
#include "cpu6502.h"
#include "pia6520.h"
#include "via6522.h"
#include "crtc6545.h"
#include "pet_video.h"
#include "keyboard_matrix_pet.h"
#include "plugin_loader/main/plugin_loader.h"
#include "cli/main/cli_interpreter.h"
#include <cstring>

static void ensureRegistriesReady() {
    PluginLoader::instance().loadFromDir("./lib");
}

TEST_CASE(PET_2001_Setup) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create pet2001");

    ASSERT(ctx.machine != nullptr);
    ASSERT(ctx.machine->machineId == "pet2001");

    auto* bus = ctx.machine->buses[0].bus;
    // Test RAM
    bus->write8(0x1000, 0x42);
    ASSERT(bus->read8(0x1000) == 0x42);

    // Test Video RAM
    bus->write8(0x8000, 0x55);
    ASSERT(bus->read8(0x8000) == 0x55);
    
}

TEST_CASE(PET_4032_Setup) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create pet4032");

    ASSERT(ctx.machine != nullptr);
    ASSERT(ctx.machine->machineId == "pet4032");
    
}

TEST_CASE(PET_8032_Setup) {
    ensureRegistriesReady();
    CliContext ctx;
    CliInterpreter cli(ctx, [](const std::string&){});
    cli.processLine("create pet8032");

    ASSERT(ctx.machine != nullptr);
    ASSERT(ctx.machine->machineId == "pet8032");
    
}

TEST_CASE(IEEE488_Bus_Smoke) {
    SimpleIEEE488Bus bus;
    
    // Initial state: all high (logic 0)
    ASSERT(bus.getSignal(IEEE488Bus::ATN) == true);
    
    bus.setSignal(IEEE488Bus::ATN, false);
    ASSERT(bus.getSignal(IEEE488Bus::ATN) == false);
    
    bus.setData(0xAA);
    ASSERT(bus.getData() == 0xAA);
}
