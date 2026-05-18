#include "test_harness.h"
#include "libtoolchain/main/symbol_table.h"
#include "libtoolchain/main/source_map.h"
#include "libtoolchain/main/toolchain_registry.h"

TEST_CASE(toolchain_symbol_table) {
    SymbolTable symbols;
    
    symbols.addSymbol(0x1000, "start");
    symbols.addSymbol(0x1005, "loop");
    symbols.addSymbol(0x2000, "data");
    
    ASSERT(symbols.getLabel(0x1000) == "start");
    ASSERT(symbols.getLabel(0x1005) == "loop");
    ASSERT(symbols.getLabel(0x3000) == "");
    
    uint32_t offset = 0;
    ASSERT(symbols.nearest(0x1007, offset) == "loop");
    ASSERT(offset == 2);
    
    ASSERT(symbols.hasSymbol(0x1000));
    ASSERT(!symbols.hasSymbol(0x1234));
}

TEST_CASE(toolchain_source_map) {
    SourceMap sm;
    ASSERT(true);
}

TEST_CASE(toolchain_registry) {
    auto& reg = ToolchainRegistry::instance();
    
    class MockAssem : public IAssembler {
    public:
        const char* name() const override { return "Mock"; }
        bool isaSupported(const std::string& isa) const override { return isa == "mock"; }
        AssemblerResult assemble(const std::string&, const std::string&) override { 
            AssemblerResult res;
            res.success = true;
            return res;
        }
    };

    class MockDisasm : public IDisassembler {
    public:
        const char* isaName() const override { return "mock"; }
        int disasmOne(IBus*, uint32_t, char*, int) override { return 1; }
        int disasmEntry(IBus*, uint32_t, DisasmEntry&) override { return 1; }
        void setSymbolTable(SymbolTable*) override {}
    };
    
    reg.registerToolchain("mock", []() { return new MockDisasm(); }, []() { return new MockAssem(); });
    
    IAssembler* assem = reg.createAssembler("mock");
    ASSERT(assem != nullptr);
    ASSERT(std::string(assem->name()) == "Mock");
    delete assem;

    IDisassembler* disasm = reg.createDisassembler("mock");
    ASSERT(disasm != nullptr);
    ASSERT(std::string(disasm->isaName()) == "mock");
    delete disasm;
}

namespace {

class StubAssembler : public IAssembler {
public:
    explicit StubAssembler(const char* n) : m_name(n) {}
    const char* name() const override { return m_name; }
    bool isaSupported(const std::string&) const override { return true; }
    AssemblerResult assemble(const std::string&, const std::string&) override {
        return {true, "", "", "", "", 0, 0};
    }
private:
    const char* m_name;
};

class StubDisassembler : public IDisassembler {
public:
    const char* isaName() const override { return "stub"; }
    int disasmOne(IBus*, uint32_t, char*, int) override { return 1; }
    int disasmEntry(IBus*, uint32_t, DisasmEntry&) override { return 1; }
    void setSymbolTable(SymbolTable*) override {}
};

} // namespace

TEST_CASE(toolchain_registry_set_instance) {
    auto* original = &ToolchainRegistry::instance();

    // setInstance(nullptr) then instance() creates a fresh one
    ToolchainRegistry::setInstance(nullptr);
    auto* fresh = &ToolchainRegistry::instance();
    ASSERT(fresh != nullptr);

    // Restore
    ToolchainRegistry::setInstance(original);
    ASSERT(&ToolchainRegistry::instance() == original);
}

TEST_CASE(toolchain_registry_named_assembler) {
    auto& reg = ToolchainRegistry::instance();

    reg.registerAssemblerByName("testAsm", []() -> IAssembler* {
        return new StubAssembler("testAsm");
    });

    // Lookup by name
    IAssembler* a = reg.createAssemblerByName("testAsm");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "testAsm");
    delete a;

    // Unknown name
    ASSERT(reg.createAssemblerByName("noSuchAsm") == nullptr);

}

TEST_CASE(toolchain_registry_get_assembler_names) {
    auto& reg = ToolchainRegistry::instance();

    // Register a named assembler
    reg.registerAssemblerByName("namedAsm", []() -> IAssembler* {
        return new StubAssembler("namedAsm");
    });

    // Register an ISA-default assembler
    reg.registerToolchain("testisa",
        []() -> IDisassembler* { return new StubDisassembler(); },
        []() -> IAssembler* { return new StubAssembler("isaDefault"); });

    auto names = reg.getAssemblerNames();
    ASSERT(names.size() >= 2);

    bool hasNamed = false, hasIsa = false;
    for (const auto& n : names) {
        if (n == "namedAsm") hasNamed = true;
        if (n == "isaDefault") hasIsa = true;
    }
    ASSERT(hasNamed);
    ASSERT(hasIsa);

}

TEST_CASE(toolchain_registry_get_names_no_duplicates) {
    auto& reg = ToolchainRegistry::instance();

    // Register same name both ways — should appear only once
    reg.registerAssemblerByName("dupeAsm", []() -> IAssembler* {
        return new StubAssembler("dupeAsm");
    });
    reg.registerToolchain("dupeisa",
        nullptr,
        []() -> IAssembler* { return new StubAssembler("dupeAsm"); });

    auto names = reg.getAssemblerNames();
    int count = 0;
    for (const auto& n : names) {
        if (n == "dupeAsm") count++;
    }
    ASSERT_EQ(count, 1);

}

TEST_CASE(toolchain_registry_create_unknown_isa) {
    auto& reg = ToolchainRegistry::instance();
    ASSERT(reg.createAssembler("_no_such_isa_") == nullptr);
    ASSERT(reg.createDisassembler("_no_such_isa_") == nullptr);
}

TEST_CASE(toolchain_resolve_isa_default) {
    auto& reg = ToolchainRegistry::instance();

    reg.registerToolchain("6502",
        []() -> IDisassembler* { return new StubDisassembler(); },
        []() -> IAssembler* { return new StubAssembler("asm6502"); });

    // No override, no machine preferred — falls back to ISA default
    IAssembler* a = resolveAssembler("6502", "", "");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "asm6502");
    delete a;

}

TEST_CASE(toolchain_resolve_machine_preferred) {
    auto& reg = ToolchainRegistry::instance();

    reg.registerToolchain("6502",
        nullptr,
        []() -> IAssembler* { return new StubAssembler("isaDefault"); });
    reg.registerAssemblerByName("preferred", []() -> IAssembler* {
        return new StubAssembler("preferred");
    });

    // Machine preferred wins over ISA default
    IAssembler* a = resolveAssembler("6502", "preferred", "");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "preferred");
    delete a;

}

TEST_CASE(toolchain_resolve_runtime_override) {
    auto& reg = ToolchainRegistry::instance();

    reg.registerToolchain("6502",
        nullptr,
        []() -> IAssembler* { return new StubAssembler("isaDefault"); });
    reg.registerAssemblerByName("preferred", []() -> IAssembler* {
        return new StubAssembler("preferred");
    });
    reg.registerAssemblerByName("override", []() -> IAssembler* {
        return new StubAssembler("override");
    });

    // Runtime override wins over everything
    IAssembler* a = resolveAssembler("6502", "preferred", "override");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "override");
    delete a;

}

TEST_CASE(toolchain_resolve_nothing_available) {
    auto& reg = ToolchainRegistry::instance();

    // No assemblers registered at all
    IAssembler* a = resolveAssembler("unknown", "", "");
    ASSERT(a == nullptr);

}

TEST_CASE(toolchain_resolve_fallback_chain) {
    auto& reg = ToolchainRegistry::instance();

    reg.registerToolchain("6502",
        nullptr,
        []() -> IAssembler* { return new StubAssembler("isaDefault"); });

    // Machine preferred name doesn't exist — falls back to ISA default
    IAssembler* a = resolveAssembler("6502", "nonexistent", "");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "isaDefault");
    delete a;

    // Runtime override doesn't exist, machine preferred doesn't exist — ISA default
    a = resolveAssembler("6502", "nope", "alsoNope");
    ASSERT(a != nullptr);
    ASSERT(std::string(a->name()) == "isaDefault");
    delete a;

}
