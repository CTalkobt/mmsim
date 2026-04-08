#pragma once

#include <string>
#include <cstdint>

class DebugContext;

class ExpressionEvaluator {
public:
    /**
     * Evaluates a mathematical expression or a single value (hex, binary, decimal, or symbol).
     * @param expression The string to evaluate (e.g., "$1000", "start + 5", "%1010").
     * @param dbg Debug context for symbol lookup and machine state.
     * @param result Output parameter for the evaluated value.
     * @return true if evaluation succeeded, false otherwise.
     */
    static bool evaluate(const std::string& expression, DebugContext* dbg, uint32_t& result);

    /**
     * Evaluates a condition expression.
     * @return true if the condition is met (non-zero), false otherwise.
     */
    static bool evaluateCondition(const std::string& expression, DebugContext* dbg);
};
