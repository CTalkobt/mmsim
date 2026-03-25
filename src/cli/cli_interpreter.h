#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <functional>
#include "libcore/machine_desc.h"
#include "libtoolchain/idisasm.h"
#include "libtoolchain/iassembler.h"
#include "libdebug/debug_context.h"

struct CliContext {
    MachineDescriptor* machine = nullptr;
    ICore* cpu = nullptr;
    IBus* bus = nullptr;
    IDisassembler* disasm = nullptr;
    IAssembler* assem = nullptr;
    DebugContext* dbg = nullptr;
    bool quit = false;
};

class CliInterpreter {
public:
    using OutputFn = std::function<void(const std::string&)>;

    CliInterpreter(CliContext& ctx, OutputFn output) : m_ctx(ctx), m_output(output) {}

    void processLine(const std::string& line);
    bool isAssemblyMode() const { return m_asmMode; }
    uint32_t getAsmAddr() const { return m_asmAddr; }

private:
    void handleNormalCommand(const std::string& line);
    void handleAssemblyLine(const std::string& line);
    void printHelp();
    void dumpMemory(uint32_t addr, uint32_t len);
    void showRegisters();

    CliContext& m_ctx;
    OutputFn m_output;
    bool m_asmMode = false;
    uint32_t m_asmAddr = 0;
};
