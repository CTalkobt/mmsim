#include "mmemu_plugin_api.h"
#include "cpu6502.h"
#include "disassembler_6502.h"
#include "assembler_6502.h"
#include "libcore/main/machine_desc.h"
#include "libmem/main/memory_bus.h"

static ICore* createCore6502() {
    return new MOS6502();
}

static IDisassembler* createDisasm6502() {
    return new Disassembler6502();
}

static IAssembler* createAsm6502() {
    return new Assembler6502();
}

static MachineDescriptor* createMachineRaw6502() {
    MachineDescriptor* desc = new MachineDescriptor();
    desc->machineId = "raw6502";
    desc->displayName = "Raw 6502 System (Plugin)";
    
    FlatMemoryBus* bus = new FlatMemoryBus("system", 16);
    MOS6502* cpu = new MOS6502();
    cpu->setDataBus(bus);
    
    desc->cpus.push_back({"main", cpu, bus, bus, nullptr, true, 1});
    desc->buses.push_back({"system", bus});
    
    return desc;
}

static CorePluginInfo s_cores[] = {
    {"6502", "NMOS", "open", createCore6502}
};

static ToolchainPluginInfo s_toolchains[] = {
    {"6502", createDisasm6502, createAsm6502}
};

static MachinePluginInfo s_machines[] = {
    {"raw6502", createMachineRaw6502}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-6502",
    "1.0.0",
    1, s_cores,
    1, s_toolchains,
    0, nullptr,
    1, s_machines
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    (void)host;
    return &s_manifest;
}
