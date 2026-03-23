#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "libcore/machine_desc.h"
#include "libcore/machines/machine_registry.h"
#include "libcore/core_registry.h"
#include "libtoolchain/toolchain_registry.h"
#include "plugin_loader/plugin_loader.h"
#include "libmem/memory_bus.h"
#include "libdebug/debug_context.h"

// Simple CLI for mmemu

struct CliContext {
    MachineDescriptor* machine = nullptr;
    ICore* cpu = nullptr;
    IBus* bus = nullptr;
    IDisassembler* disasm = nullptr;
    IAssembler* asm6502 = nullptr;
    DebugContext* dbg = nullptr;
    bool quit = false;
};

void printHelp() {
    std::cout << "Available commands:\n"
              << "  help, ?          - Show this help\n"
              << "  list             - List available machine types\n"
              << "  create <id>      - Create a machine of the given type\n"
              << "  step [n]         - Step the CPU N times (default 1)\n"
              << "  run              - Run until a breakpoint or stop\n"
              << "  regs             - Show CPU registers\n"
              << "  m <addr> [len]   - Dump memory\n"
              << "  f <addr> <val>   - Fill memory byte\n"
              << "  disasm <addr> [n]- Disassemble N instructions\n"
              << "  load <path> <addr> - Load a binary file into memory\n"
              << "  .<instr>         - Assemble and execute a single instruction (e.g. .lda #$02)\n"
              << "  quit, q          - Exit the program\n";
}

void dumpMemory(IBus* bus, uint32_t addr, uint32_t len) {
    for (uint32_t i = 0; i < len; i += 16) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << (addr + i) << ": ";
        for (uint32_t j = 0; j < 16 && (i + j) < len; ++j) {
            std::cout << std::setw(2) << (int)bus->read8(addr + i + j) << " ";
        }
        std::cout << std::dec << std::endl;
    }
}

void showRegisters(ICore* cpu) {
    int count = cpu->regCount();
    for (int i = 0; i < count; ++i) {
        const auto* desc = cpu->regDescriptor(i);
        if (desc->flags & REGFLAG_INTERNAL) continue;
        
        uint32_t val = cpu->regRead(i);
        std::cout << desc->name << ": ";
        if (desc->width == RegWidth::R16) {
            std::cout << "$" << std::hex << std::setw(4) << std::setfill('0') << val;
        } else {
            std::cout << "$" << std::hex << std::setw(2) << std::setfill('0') << val;
        }
        std::cout << "  ";
        if ((i + 1) % 4 == 0) std::cout << "\n";
    }
    std::cout << std::dec << "\nCycles: " << cpu->cycles() << std::endl;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    std::cout << "mmemu - Multi Machine Emulator (CLI)\n";
    std::cout << "Version 0.1.0-dev\n";
    
    // Initialize plugins
    PluginLoader::instance().loadFromDir("./lib");

    std::cout << "Type 'help' for a list of commands.\n";

    CliContext ctx;
    
    std::string line;
    while (!ctx.quit) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        if (line[0] == '.') {
            if (!ctx.cpu || !ctx.asm6502) { 
                std::cout << "No machine created or no assembler for this ISA.\n"; 
                continue; 
            }
            std::string instr = line.substr(1);
            uint8_t opcodes[16];
            int sz = ctx.asm6502->assembleLine(instr, opcodes, sizeof(opcodes));
            if (sz > 0) {
                uint32_t scratch = 0x0200; // Common scratch area
                for (int i = 0; i < sz; ++i) ctx.bus->write8(scratch + i, opcodes[i]);
                
                uint32_t oldPc = ctx.cpu->pc();
                ctx.cpu->setPc(scratch);
                ctx.cpu->step();
                ctx.cpu->setPc(oldPc);
                showRegisters(ctx.cpu);
            } else {
                std::cout << "Assembly failed: " << instr << "\n";
            }
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "help" || cmd == "?") {
            printHelp();
        } else if (cmd == "list") {
            std::vector<std::string> ids;
            MachineRegistry::instance().enumerate(ids);
            std::cout << "Available machines:\n";
            for (const auto& id : ids) std::cout << "  " << id << "\n";
        } else if (cmd == "create") {
            std::string id;
            if (ss >> id) {
                ctx.machine = MachineRegistry::instance().createMachine(id);
                if (ctx.machine) {
                    ctx.cpu = ctx.machine->cpus[0].cpu;
                    ctx.bus = ctx.machine->buses[0].bus;
                    
                    // Request toolchain from registry
                    ctx.disasm = ToolchainRegistry::instance().createDisassembler(ctx.cpu->isaName());
                    ctx.asm6502 = ToolchainRegistry::instance().createAssembler(ctx.cpu->isaName());
                    
                    ctx.dbg = new DebugContext(ctx.cpu, ctx.bus);
                    ctx.cpu->observer = ctx.dbg;
                    ctx.bus->observer = ctx.dbg;
                    std::cout << "Created machine: " << ctx.machine->displayName << "\n";
                } else {
                    std::cout << "Unknown machine type: " << id << "\n";
                }
            }
        } else if (cmd == "step") {
            if (!ctx.cpu) { std::cout << "No machine created.\n"; continue; }
            int n = 1;
            if (ss >> n) {} else { n = 1; }
            for (int i = 0; i < n; ++i) ctx.cpu->step();
            showRegisters(ctx.cpu);
        } else if (cmd == "regs") {
            if (!ctx.cpu) { std::cout << "No machine created.\n"; continue; }
            showRegisters(ctx.cpu);
        } else if (cmd == "m") {
            if (!ctx.bus) { std::cout << "No machine created.\n"; continue; }
            std::string addrStr;
            uint32_t addr = 0, len = 64;
            if (ss >> addrStr) {
                addr = std::stoul(addrStr, nullptr, 16);
                if (ss >> addrStr) len = std::stoul(addrStr, nullptr, 16);
                dumpMemory(ctx.bus, addr, len);
            }
        } else if (cmd == "f") {
            if (!ctx.bus) { std::cout << "No machine created.\n"; continue; }
            std::string addrStr, valStr;
            if (ss >> addrStr >> valStr) {
                uint32_t addr = std::stoul(addrStr, nullptr, 16);
                uint8_t val = (uint8_t)std::stoul(valStr, nullptr, 16);
                ctx.bus->write8(addr, val);
            }
        } else if (cmd == "disasm") {
            if (!ctx.cpu || !ctx.disasm) { std::cout << "No machine created.\n"; continue; }
            std::string addrStr;
            uint32_t addr = ctx.cpu->pc();
            int n = 10;
            if (ss >> addrStr) {
                addr = std::stoul(addrStr, nullptr, 16);
                if (ss >> n) {}
            }
            for (int i = 0; i < n; ++i) {
                char buf[64];
                std::cout << std::hex << std::setw(4) << std::setfill('0') << addr << ": ";
                int bytes = ctx.disasm->disasmOne(ctx.bus, addr, buf, sizeof(buf));
                std::cout << buf << "\n";
                addr += bytes;
            }
            std::cout << std::dec;
        } else if (cmd == "load") {
            std::cout << "Load not fully implemented in CLI yet.\n";
        } else if (cmd == "quit" || cmd == "q") {
            ctx.quit = true;
        } else {
            std::cout << "Unknown command: " << cmd << ". Type 'help' for info.\n";
        }
    }

    PluginLoader::instance().unloadAll();
    return 0;
}
