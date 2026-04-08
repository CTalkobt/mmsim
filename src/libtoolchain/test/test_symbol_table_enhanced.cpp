#include "test_harness.h"
#include "libtoolchain/main/symbol_table.h"
#include <fstream>
#include <cstdio>

TEST_CASE(symbol_table_management) {
    SymbolTable syms;
    
    syms.addSymbol(0x1000, "main");
    syms.addSymbol(0x2000, "data");
    
    EXPECT_EQ(syms.getLabel(0x1000), "main");
    EXPECT_EQ(syms.getAddress("data"), 0x2000);
    
    syms.removeSymbol("main");
    EXPECT_EQ(syms.getLabel(0x1000), "");
    EXPECT_EQ(syms.getAddress("main"), 0xFFFFFFFF);
    
    syms.clear();
    EXPECT_EQ(syms.symbols().size(), 0);
}

TEST_CASE(symbol_table_nearest) {
    SymbolTable syms;
    syms.addSymbol(0x1000, "start");
    syms.addSymbol(0x1010, "next");
    
    uint32_t offset;
    EXPECT_EQ(syms.nearest(0x1005, offset), "start");
    EXPECT_EQ(offset, 5);
    
    EXPECT_EQ(syms.nearest(0x1010, offset), "next");
    EXPECT_EQ(offset, 0);
    
    EXPECT_EQ(syms.nearest(0x0FFF, offset), "");
}

TEST_CASE(symbol_table_load_sym) {
    const char* filename = "test_load.sym";
    std::ofstream f(filename);
    f << "label1 = $1000\n";
    f << "label2 2000\n";
    f << "  label3 = %1010  \n";
    f.close();
    
    SymbolTable syms;
    EXPECT_TRUE(syms.loadSym(filename));
    
    EXPECT_EQ(syms.getAddress("label1"), 0x1000);
    EXPECT_EQ(syms.getAddress("label2"), 2000);
    EXPECT_EQ(syms.getAddress("label3"), 10);
    
    std::remove(filename);
}
