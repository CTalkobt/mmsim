#include "test_harness.h"
#include "libdebug/main/expression_evaluator.h"
#include "libdebug/main/debug_context.h"
#include "plugins/6502/main/cpu6502.h"
#include "libmem/main/memory_bus.h"

TEST_CASE(expression_evaluator_literals) {
    uint32_t res;
    
    // Hex
    EXPECT_TRUE(ExpressionEvaluator::evaluate("$1234", nullptr, res));
    EXPECT_EQ(res, 0x1234);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("0xABCD", nullptr, res));
    EXPECT_EQ(res, 0xABCD);
    
    // Binary
    EXPECT_TRUE(ExpressionEvaluator::evaluate("%10101010", nullptr, res));
    EXPECT_EQ(res, 0xAA);
    
    // Decimal
    EXPECT_TRUE(ExpressionEvaluator::evaluate("1234", nullptr, res));
    EXPECT_EQ(res, 1234);
    
    // Malformed
    ASSERT(!ExpressionEvaluator::evaluate("$GHIJ", nullptr, res));
    ASSERT(!ExpressionEvaluator::evaluate("%2345", nullptr, res));
}

TEST_CASE(expression_evaluator_arithmetic) {
    uint32_t res;
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 + 20", nullptr, res));
    EXPECT_EQ(res, 30);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("$1000 - $10", nullptr, res));
    EXPECT_EQ(res, 0x0FF0);
    
    // Chained
    EXPECT_TRUE(ExpressionEvaluator::evaluate("100 + 50 - 10", nullptr, res));
    EXPECT_EQ(res, 140);
    
    // Left-to-right (100 - 50 + 10 = 60, not 40)
    EXPECT_TRUE(ExpressionEvaluator::evaluate("100 - 50 + 10", nullptr, res));
    EXPECT_EQ(res, 60);
}

TEST_CASE(expression_evaluator_symbols_regs) {
    FlatMemoryBus bus("test", 16);
    MOS6502 cpu;
    DebugContext dbg(&cpu, &bus);
    uint32_t res;
    
    // Symbols
    dbg.symbols().addSymbol(0x1234, "start");
    EXPECT_TRUE(ExpressionEvaluator::evaluate("start", &dbg, res));
    EXPECT_EQ(res, 0x1234);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("start + $10", &dbg, res));
    EXPECT_EQ(res, 0x1244);
    
    // Registers
    cpu.setPc(0x4000);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("PC", &dbg, res));
    EXPECT_EQ(res, 0x4000);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("PC + 5", &dbg, res));
    EXPECT_EQ(res, 0x4005);
}
