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
     * @param defaultBase The base to use if no prefix is present (10 by default).
     * @return true if evaluation succeeded, false otherwise.
     */
    static bool evaluate(const std::string& expression, DebugContext* dbg, uint32_t& result, int defaultBase = 10);

    /**
     * Evaluates a mathematical expression or a single value returning a double.
     * Supports floating point literals and functions like sin, cos, sqrt, log.
     */
    static bool evaluate(const std::string& expression, DebugContext* dbg, double& result, int defaultBase = 10);

    /**
     * Evaluates a condition expression.
     * @return true if the condition is met (non-zero), false otherwise.
     */
    static bool evaluateCondition(const std::string& expression, DebugContext* dbg, int defaultBase = 10);
};
