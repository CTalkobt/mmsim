#include "cli_interpreter.h"
#include "libcore/machines/machine_registry.h"
#include "libtoolchain/toolchain_registry.h"
#include <iostream>

void CliInterpreter::processLine(const std::string& line) {
    if (m_asmMode) {
        handleAssemblyLine(line);
    } else {
        handleNormalCommand(line);
    }
}

void CliInterpreter::handleNormalCommand(const std::string& line) {
    if (line.empty()) return;

    if (line[0] == '.') {
        if (!m_ctx.cpu || !m_ctx.assem) {
            m_output("No machine created or no assembler for this ISA.\n");
            return;
        }
        std::string instr = line.substr(1);
        uint8_t opcodes[16];
        int sz = m_ctx.assem->assembleLine(instr, opcodes, sizeof(opcodes), m_ctx.cpu->pc());
        if (sz > 0) {
            uint32_t scratch = 0x0200;
            for (int i = 0; i < sz; ++i) m_ctx.bus->write8(scratch + i, opcodes[i]);
            uint32_t oldPc = m_ctx.cpu->pc();
            m_ctx.cpu->setPc(scratch);
            m_ctx.cpu->step();
            m_ctx.cpu->setPc(oldPc);
            showRegisters();
        } else {
            m_output("Assembly failed: " + instr + "\n");
        }
        return;
    }

    std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd == "list") {
        std::vector<std::string> ids;
        MachineRegistry::instance().enumerate(ids);
        m_output("Available machines:\n");
        for (const auto& id : ids) m_output("  " + id + "\n");
    } else if (cmd == "create") {
        std::string id;
        if (ss >> id) {
            m_ctx.machine = MachineRegistry::instance().createMachine(id);
            if (m_ctx.machine) {
                m_ctx.cpu = m_ctx.machine->cpus[0].cpu;
                m_ctx.bus = m_ctx.machine->buses[0].bus;
                m_ctx.disasm = ToolchainRegistry::instance().createDisassembler(m_ctx.cpu->isaName());
                m_ctx.assem = ToolchainRegistry::instance().createAssembler(m_ctx.cpu->isaName());
                m_ctx.dbg = new DebugContext(m_ctx.cpu, m_ctx.bus);
                m_ctx.cpu->observer = m_ctx.dbg;
                m_ctx.bus->observer = m_ctx.dbg;
                m_output("Created machine: " + m_ctx.machine->displayName + "\n");
            } else {
                m_output("Unknown machine type: " + id + "\n");
            }
        }
    } else if (cmd == "step") {
        if (!m_ctx.cpu) { m_output("No machine created.\n"); return; }
        int n = 1;
        if (ss >> n) {} else { n = 1; }
        for (int i = 0; i < n; ++i) m_ctx.cpu->step();
        showRegisters();
    } else if (cmd == "regs") {
        if (!m_ctx.cpu) { m_output("No machine created.\n"); return; }
        showRegisters();
    } else if (cmd == "m") {
        if (!m_ctx.bus) { m_output("No machine created.\n"); return; }
        std::string addrStr;
        uint32_t addr = 0, len = 64;
        if (ss >> addrStr) {
            addr = std::stoul(addrStr, nullptr, 16);
            if (ss >> addrStr) len = std::stoul(addrStr, nullptr, 16);
            dumpMemory(addr, len);
        }
    } else if (cmd == "f") {
        if (!m_ctx.bus) { m_output("No machine created.\n"); return; }
        std::string addrStr, valStr, lenStr;
        if (ss >> addrStr >> valStr) {
            uint32_t addr = std::stoul(addrStr, nullptr, 16);
            uint8_t val = (uint8_t)std::stoul(valStr, nullptr, 16);
            uint32_t len = 1;
            if (ss >> lenStr) len = std::stoul(lenStr, nullptr, 16);
            for (uint32_t i = 0; i < len; ++i) m_ctx.bus->write8(addr + i, val);
        }
    } else if (cmd == "copy") {
        if (!m_ctx.bus) { m_output("No machine created.\n"); return; }
        std::string srcStr, dstStr, lenStr;
        if (ss >> srcStr >> dstStr >> lenStr) {
            uint32_t src = std::stoul(srcStr, nullptr, 16);
            uint32_t dst = std::stoul(dstStr, nullptr, 16);
            uint32_t len = std::stoul(lenStr, nullptr, 16);
            std::vector<uint8_t> tmp(len);
            for (uint32_t i = 0; i < len; ++i) tmp[i] = m_ctx.bus->read8(src + i);
            for (uint32_t i = 0; i < len; ++i) m_ctx.bus->write8(dst + i, tmp[i]);
        }
    } else if (cmd == "disasm") {
        if (!m_ctx.cpu || !m_ctx.disasm) { m_output("No machine created.\n"); return; }
        std::string addrStr;
        uint32_t addr = m_ctx.cpu->pc();
        int n = 10;
        if (ss >> addrStr) {
            addr = std::stoul(addrStr, nullptr, 16);
            if (ss >> n) {}
        }
        for (int i = 0; i < n; ++i) {
            char buf[64];
            std::stringstream res;
            res << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addr << ": ";
            int bytes = m_ctx.disasm->disasmOne(m_ctx.bus, addr, buf, sizeof(buf));
            res << buf << "\n";
            m_output(res.str());
            addr += bytes;
        }
    } else if (cmd == "asm") {
        if (!m_ctx.cpu || !m_ctx.assem) { m_output("No machine created.\n"); return; }
        std::string addrStr;
        if (ss >> addrStr) {
            m_asmAddr = std::stoul(addrStr, nullptr, 16);
            m_asmMode = true;
        } else {
            m_output("Syntax: asm <address>\n");
        }
    } else if (cmd == "quit" || cmd == "q") {
        m_ctx.quit = true;
    } else {
        m_output("Unknown command: " + cmd + ". Type 'help' for info.\n");
    }
}

void CliInterpreter::handleAssemblyLine(const std::string& line) {
    if (line == ".") {
        m_asmMode = false;
        m_output("Assembly ended.\n");
        return;
    }

    uint8_t opcodes[16];
    int sz = m_ctx.assem->assembleLine(line, opcodes, sizeof(opcodes), m_asmAddr);
    if (sz > 0) {
        std::stringstream res;
        res << "> " << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << m_asmAddr << " ";
        for (int i = 0; i < sz; ++i) {
            m_ctx.bus->write8(m_asmAddr + i, opcodes[i]);
            res << "$" << std::setw(2) << (int)opcodes[i] << " ";
        }
        res << "\n";
        m_output(res.str());
        m_asmAddr += sz;
    } else {
        m_output("Assembly failed: " + line + "\n");
    }
}

void CliInterpreter::printHelp() {
    m_output("Available commands:\n"
             "  help, ?          - Show this help\n"
             "  list             - List available machine types\n"
             "  create <id>      - Create a machine of the given type\n"
             "  step [n]         - Step the CPU N times (default 1)\n"
             "  run              - Run until a breakpoint or stop\n"
             "  regs             - Show CPU registers\n"
             "  m <addr> [len]   - Dump memory\n"
             "  f <addr> <val> [len] - Fill memory range\n"
             "  copy <src> <dst> <len> - Copy memory range\n"
             "  disasm <addr> [n]- Disassemble N instructions\n"
             "  asm <addr>       - Interactive assembly mode (end with '.')\n"
             "  load <path> <addr> - Load a binary file into memory\n"
             "  .<instr>         - Assemble and execute a single instruction\n"
             "  quit, q          - Exit the program\n");
}

void CliInterpreter::dumpMemory(uint32_t addr, uint32_t len) {
    std::stringstream res;
    for (uint32_t i = 0; i < len; i += 16) {
        res << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (addr + i) << ": ";
        for (uint32_t j = 0; j < 16 && (i + j) < len; ++j) {
            res << std::setw(2) << (int)m_ctx.bus->read8(addr + i + j) << " ";
        }
        res << "\n";
    }
    m_output(res.str());
}

void CliInterpreter::showRegisters() {
    std::stringstream res;
    int count = m_ctx.cpu->regCount();
    for (int i = 0; i < count; ++i) {
        const auto* desc = m_ctx.cpu->regDescriptor(i);
        if (desc->flags & REGFLAG_INTERNAL) continue;
        
        uint32_t val = m_ctx.cpu->regRead(i);
        res << desc->name << ": ";
        if (desc->width == RegWidth::R16) {
            res << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << val;
        } else {
            res << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << val;
        }
        res << "  ";
        if ((i + 1) % 4 == 0) res << "\n";
    }
    res << "\nCycles: " << std::dec << m_ctx.cpu->cycles() << "\n";
    m_output(res.str());
}
