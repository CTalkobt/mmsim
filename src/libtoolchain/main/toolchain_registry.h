#pragma once

#include "idisasm.h"
#include "iassembler.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

class ToolchainRegistry {
public:
    using DisasmFactoryFn = std::function<IDisassembler*()>;
    using AsmFactoryFn    = std::function<IAssembler*()>;

    struct ToolchainInfo {
        std::string     isa;
        DisasmFactoryFn disasmFactory;
        AsmFactoryFn    asmFactory;
    };

    static ToolchainRegistry& instance();

    void registerToolchain(const std::string& isa, DisasmFactoryFn df, AsmFactoryFn af);
    IDisassembler* createDisassembler(const std::string& isa);
    IAssembler*    createAssembler(const std::string& isa);

private:
    ToolchainRegistry() = default;
    std::map<std::string, ToolchainInfo> m_toolchains;
};
