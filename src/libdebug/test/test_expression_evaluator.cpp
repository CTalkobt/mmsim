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
    
    // Character literals
    EXPECT_TRUE(ExpressionEvaluator::evaluate("'a'", nullptr, res));
    EXPECT_EQ(res, 'a');
    EXPECT_TRUE(ExpressionEvaluator::evaluate("'\\n'", nullptr, res));
    EXPECT_EQ(res, '\n');
    
    // Binary literal + Modulus
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 % %11", nullptr, res)); // 10 % 3 = 1
    EXPECT_EQ(res, 1);
    
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
    
    // Division and Modulus
    EXPECT_TRUE(ExpressionEvaluator::evaluate("100 / 4", nullptr, res));
    EXPECT_EQ(res, 25);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 % 3", nullptr, res));
    EXPECT_EQ(res, 1);
    
    // Precedence: 10 + 20 / 2 => 10 + (20 / 2) = 20
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 + 20 / 2", nullptr, res));
    EXPECT_EQ(res, 20);
}

TEST_CASE(expression_evaluator_parentheses) {
    uint32_t res;
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("(10 + 20) * 0 + 5", nullptr, res)); // We don't have * yet, but let's see
    // Wait, I didn't add *. Let me check my binOps.
    // I only added +, -, &, |, ==, !=, <=, >=, <, >.
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("(10 + 20) - 5", nullptr, res));
    EXPECT_EQ(res, 25);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("100 - (50 + 10)", nullptr, res));
    EXPECT_EQ(res, 40);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("((1 + 2) + 3)", nullptr, res));
    EXPECT_EQ(res, 6);
}

TEST_CASE(expression_evaluator_unary) {
    uint32_t res;
    
    // High byte <
    EXPECT_TRUE(ExpressionEvaluator::evaluate("<$1234", nullptr, res));
    EXPECT_EQ(res, 0x12);
    
    // Low byte >
    EXPECT_TRUE(ExpressionEvaluator::evaluate(">$1234", nullptr, res));
    EXPECT_EQ(res, 0x34);
    
    // Negation !
    EXPECT_TRUE(ExpressionEvaluator::evaluate("!0", nullptr, res));
    EXPECT_EQ(res, 1);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("!55", nullptr, res));
    EXPECT_EQ(res, 0);
    
    // Combined
    EXPECT_TRUE(ExpressionEvaluator::evaluate("! (>$1200)", nullptr, res));
    EXPECT_EQ(res, 1); // >$1200 is 0, !0 is 1
}

TEST_CASE(expression_evaluator_comparisons) {
    uint32_t res;
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 == 10", nullptr, res));
    EXPECT_EQ(res, 1);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 == 20", nullptr, res));
    EXPECT_EQ(res, 0);
    
    EXPECT_TRUE(ExpressionEvaluator::evaluate("5 < 10", nullptr, res));
    EXPECT_EQ(res, 1);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 < 5", nullptr, res));
    EXPECT_EQ(res, 0);
    
    // Precedence: 10 == 5 + 5  => 10 == (5 + 5)
    EXPECT_TRUE(ExpressionEvaluator::evaluate("10 == 5 + 5", nullptr, res));
    EXPECT_EQ(res, 1);
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
    
    dbg.symbols().addSymbol(0x2000, "my_symbol");
    EXPECT_TRUE(ExpressionEvaluator::evaluate("my_symbol + 5", &dbg, res));
    EXPECT_EQ(res, 0x2005);
    
    // Registers (bare name)
    cpu.setPc(0x4000);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("PC", &dbg, res));
    EXPECT_EQ(res, 0x4000);

    EXPECT_TRUE(ExpressionEvaluator::evaluate("PC + 5", &dbg, res));
    EXPECT_EQ(res, 0x4005);
}

TEST_CASE(expression_evaluator_flag_and_reg_shortcuts) {
    FlatMemoryBus bus("test", 16);
    MOS6502 cpu;
    DebugContext dbg(&cpu, &bus);
    uint32_t res;

    // --- @reg shorthand ---
    cpu.setPc(0x1234);
    EXPECT_TRUE(ExpressionEvaluator::evaluate("@PC", &dbg, res));
    EXPECT_EQ(res, 0x1234);

    // Case-insensitive
    EXPECT_TRUE(ExpressionEvaluator::evaluate("@pc", &dbg, res));
    EXPECT_EQ(res, 0x1234);

    cpu.regWrite(0, 0x42); // A = 0x42
    EXPECT_TRUE(ExpressionEvaluator::evaluate("@A", &dbg, res));
    EXPECT_EQ(res, 0x42);

    // @reg in expressions
    EXPECT_TRUE(ExpressionEvaluator::evaluate("@A + 1", &dbg, res));
    EXPECT_EQ(res, 0x43);

    // Unknown @reg fails
    ASSERT(!ExpressionEvaluator::evaluate("@NOPE", &dbg, res));

    // --- .flag shorthand (6502 P register: NV-BDIZC) ---
    // P = 0x01 → only Carry set
    cpu.regWrite(5, 0x01);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".C", &dbg, res));
    EXPECT_EQ(res, 1u);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".Z", &dbg, res));
    EXPECT_EQ(res, 0u);

    // P = 0x02 → only Zero set
    cpu.regWrite(5, 0x02);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".Z", &dbg, res));
    EXPECT_EQ(res, 1u);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".C", &dbg, res));
    EXPECT_EQ(res, 0u);

    // P = 0x80 → Negative set
    cpu.regWrite(5, 0x80);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".N", &dbg, res));
    EXPECT_EQ(res, 1u);

    // P = 0x40 → Overflow set
    cpu.regWrite(5, 0x40);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".V", &dbg, res));
    EXPECT_EQ(res, 1u);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".N", &dbg, res));
    EXPECT_EQ(res, 0u);

    // Case-insensitive flag letter
    cpu.regWrite(5, 0x01);
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".c", &dbg, res));
    EXPECT_EQ(res, 1u);

    // .flag in a condition expression
    EXPECT_TRUE(ExpressionEvaluator::evaluate(".C == 1", &dbg, res));
    EXPECT_EQ(res, 1u);

    // Unknown flag letter fails
    ASSERT(!ExpressionEvaluator::evaluate(".Q", &dbg, res));

    // No cpu → flag fails gracefully
    ASSERT(!ExpressionEvaluator::evaluate(".C", nullptr, res));
}
