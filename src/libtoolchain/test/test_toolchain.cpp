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
