#pragma once

#include <string>

class ICore;
class IBus;

class ExpressionEvaluator {
public:
    bool evaluate(const std::string& expression, ICore* cpu, IBus* bus);
};
