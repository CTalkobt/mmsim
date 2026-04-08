#include "expression_evaluator.h"
#include "debug_context.h"
#include "libcore/main/icore.h"
#include <algorithm>
#include <cctype>

static std::string trim(const std::string& s) {
    auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch); });
    auto last  = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    return (first < last) ? std::string(first, last) : "";
}

bool ExpressionEvaluator::evaluate(const std::string& expression, DebugContext* dbg, uint32_t& result) {
    std::string expr = trim(expression);
    if (expr.empty()) return false;

    // Find the last + or - to handle left-to-right evaluation (a - b + c)
    size_t lastPlus = expr.rfind('+');
    size_t lastMinus = expr.rfind('-');
    
    size_t opPos = std::string::npos;
    char op = 0;

    if (lastPlus != std::string::npos && lastMinus != std::string::npos) {
        if (lastPlus > lastMinus) { opPos = lastPlus; op = '+'; }
        else { opPos = lastMinus; op = '-'; }
    } else if (lastPlus != std::string::npos) {
        opPos = lastPlus; op = '+';
    } else if (lastMinus != std::string::npos) {
        // Distinguish between binary minus and unary minus/negation in hex ($dead-beef vs -5)
        // For now, only support binary minus if there's something to the left.
        if (lastMinus > 0) {
            opPos = lastMinus; op = '-';
        }
    }

    if (opPos != std::string::npos) {
        uint32_t a, b;
        if (evaluate(expr.substr(0, opPos), dbg, a) && evaluate(expr.substr(opPos + 1), dbg, b)) {
            if (op == '+') result = a + b;
            else result = a - b;
            return true;
        }
        return false;
    }

    // Literal values
    if (expr[0] == '$') {
        try {
            result = std::stoul(expr.substr(1), nullptr, 16);
            return true;
        } catch (...) { return false; }
    } else if (expr[0] == '%') {
        try {
            result = std::stoul(expr.substr(1), nullptr, 2);
            return true;
        } catch (...) { return false; }
    } else if (std::isdigit(expr[0])) {
        try {
            // stoul with base 0 handles 0x for hex, 0 for octal, else decimal.
            result = std::stoul(expr, nullptr, 0);
            return true;
        } catch (...) { return false; }
    }

    // Symbol or Register
    if (dbg) {
        // Try Symbol Table
        uint32_t symAddr = dbg->symbols().getAddress(expr);
        if (symAddr != 0xFFFFFFFF) {
            result = symAddr;
            return true;
        }

        // Try Register
        if (dbg->cpu()) {
            // Case-insensitive register names for convenience
            std::string upperName = expr;
            std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
            
            int regIdx = dbg->cpu()->regIndexByName(upperName.c_str());
            if (regIdx != -1) {
                result = dbg->cpu()->regRead(regIdx);
                return true;
            }
        }
    }

    return false;
}

bool ExpressionEvaluator::evaluateCondition(const std::string& expression, DebugContext* dbg) {
    if (expression.empty()) return true;
    uint32_t result;
    if (evaluate(expression, dbg, result)) {
        return result != 0;
    }
    return false;
}
