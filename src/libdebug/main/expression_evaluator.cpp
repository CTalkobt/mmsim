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

static bool removeOuterParentheses(std::string& expr) {
    if (expr.empty() || expr[0] != '(' || expr.back() != ')') return false;
    
    int depth = 0;
    for (size_t i = 0; i < expr.length() - 1; ++i) {
        if (expr[i] == '(') depth++;
        else if (expr[i] == ')') depth--;
        if (depth == 0) return false; // Parentheses closed before end
    }
    
    expr = trim(expr.substr(1, expr.length() - 2));
    return true;
}

bool ExpressionEvaluator::evaluate(const std::string& expression, DebugContext* dbg, uint32_t& result) {
    std::string expr = trim(expression);
    while (removeOuterParentheses(expr));
    if (expr.empty()) return false;

    struct OpInfo { std::string s; int precedence; };
    static const std::vector<OpInfo> binOps = {
        {"==", 1}, {"!=", 1}, {"<=", 1}, {">=", 1}, {"<",  1}, {">",  1},
        {"|",  2},
        {"&",  3},
        {"+",  4}, {"-",  4},
        {"*",  5}, {"/",  5}, {"%",  5}
    };

    // Find the operator of the lowest precedence at depth 0.
    // For same precedence, we want the LEFTMOST one if we were doing iterative, 
    // but for recursive descent splitting, we actually want the RIGHTMOST one 
    // to achieve left-to-right associativity.
    // Wait, let's re-think:
    // 100 - 50 + 10
    // If we split at '+', we get evaluate(100 - 50) + evaluate(10).
    // evaluate(100 - 50) = 50.
    // 50 + 10 = 60. Correct.
    // So for left-to-right associativity, we must split at the RIGHTMOST operator of lowest precedence.
    
    // 10 % %11
    // If we split at the first '%', evaluate(10) % evaluate(%11).
    // evaluate(%11) = 3.
    // 10 % 3 = 1. Correct.
    // If we split at the second '%', evaluate(10 % ) % evaluate(11). Error.
    
    // The issue with 10 % %11 was likely that my "hasLeft" check failed for the second '%'
    // but then it was still found as a binOp? No, if hasLeft is false, it continues.
    
    // Let's re-verify the rightmost logic.
    size_t opPos = std::string::npos;
    std::string opStr;
    int bestPrec = 1000;

    int depth = 0;
    for (int i = 0; i < (int)expr.length(); ++i) {
        if (expr[i] == '(') depth++;
        else if (expr[i] == ')') depth--;
        else if (depth == 0) {
            for (const auto& op : binOps) {
                if (i + op.s.length() <= expr.length() && expr.substr(i, op.s.length()) == op.s) {
                    // Check if it's binary or unary
                    bool hasLeft = false;
                    for (int j = i - 1; j >= 0; --j) {
                        if (!std::isspace((unsigned char)expr[j])) {
                            hasLeft = true;
                            break;
                        }
                    }
                    if (!hasLeft) continue; // It's unary prefix, not a binary operator

                    // For modulus %, we require it not be followed immediately by a binary digit
                    // to avoid ambiguity with binary literal prefix (e.g., 10 % %11).
                    // This forces a space or parenthesis if the right operand starts with 0 or 1.
                    if (op.s == "%" && (size_t)i + 1 < expr.length()) {
                        char next = expr[i + 1];
                        if (next == '0' || next == '1') continue;
                    }

                    // For same precedence, we take the rightmost one to get left-to-right associativity in recursion.
                    if (op.precedence <= bestPrec) {
                        bestPrec = op.precedence;
                        opPos = i;
                        opStr = op.s;
                    }
                }
            }
        }
    }

    if (opPos != std::string::npos) {
        uint32_t a, b;
        if (evaluate(expr.substr(0, opPos), dbg, a) && evaluate(expr.substr(opPos + opStr.length()), dbg, b)) {
            if (opStr == "==") result = (a == b);
            else if (opStr == "!=") result = (a != b);
            else if (opStr == "<=") result = (a <= b);
            else if (opStr == ">=") result = (a >= b);
            else if (opStr == "<")  result = (a < b);
            else if (opStr == ">")  result = (a > b);
            else if (opStr == "|")  result = a | b;
            else if (opStr == "&")  result = a & b;
            else if (opStr == "+")  result = a + b;
            else if (opStr == "-")  result = a - b;
            else if (opStr == "*")  result = a * b;
            else if (opStr == "/") {
                if (b == 0) return false;
                result = a / b;
            } else if (opStr == "%") {
                if (b == 0) return false;
                result = a % b;
            }
            return true;
        }
        return false;
    }

    // Unary prefix operators
    if (expr[0] == '!') {
        uint32_t a;
        if (evaluate(expr.substr(1), dbg, a)) {
            result = (a == 0) ? 1 : 0;
            return true;
        }
        return false;
    }
    if (expr[0] == '<') { // High byte as per user request
        uint32_t a;
        if (evaluate(expr.substr(1), dbg, a)) {
            result = (a >> 8) & 0xFF;
            return true;
        }
        return false;
    }
    if (expr[0] == '>') { // Low byte as per user request
        uint32_t a;
        if (evaluate(expr.substr(1), dbg, a)) {
            result = a & 0xFF;
            return true;
        }
        return false;
    }

    // Literal values
    if (expr[0] == '$') {
        if (expr.length() < 2 || std::isspace((unsigned char)expr[1])) return false;
        std::string sub = expr.substr(1);
        try {
            size_t processed = 0;
            result = std::stoul(sub, &processed, 16);
            return processed == sub.length();
        } catch (...) { return false; }
    } else if (expr[0] == '%') {
        if (expr.length() < 2 || std::isspace((unsigned char)expr[1])) return false;
        std::string sub = expr.substr(1);
        try {
            size_t processed = 0;
            result = std::stoul(sub, &processed, 2);
            return processed == sub.length();
        } catch (...) { return false; }
    } else if (std::isdigit(expr[0])) {
        try {
            size_t processed = 0;
            result = std::stoul(expr, &processed, 0);
            return processed == expr.length();
        } catch (...) { return false; }
    } else if (expr.length() >= 3 && expr.front() == '\'' && expr.back() == '\'') {
        if (expr.length() == 3) {
            result = (unsigned char)expr[1];
            return true;
        } else if (expr.length() == 4 && expr[1] == '\\') {
            switch (expr[2]) {
                case 'n':  result = '\n'; return true;
                case 'r':  result = '\r'; return true;
                case 't':  result = '\t'; return true;
                case '\\': result = '\\'; return true;
                case '\'': result = '\''; return true;
                default:   return false;
            }
        }
        return false;
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
