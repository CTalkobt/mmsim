#include "toolchain_registry.h"

ToolchainRegistry& ToolchainRegistry::instance() {
    static ToolchainRegistry s_instance;
    return s_instance;
}

void ToolchainRegistry::registerToolchain(const std::string& isa, DisasmFactoryFn df, AsmFactoryFn af) {
    m_toolchains[isa] = {isa, df, af};
}

IDisassembler* ToolchainRegistry::createDisassembler(const std::string& isa) {
    if (m_toolchains.count(isa) && m_toolchains[isa].disasmFactory) {
        return m_toolchains[isa].disasmFactory();
    }
    return nullptr;
}

IAssembler* ToolchainRegistry::createAssembler(const std::string& isa) {
    if (m_toolchains.count(isa) && m_toolchains[isa].asmFactory) {
        return m_toolchains[isa].asmFactory();
    }
    return nullptr;
}
