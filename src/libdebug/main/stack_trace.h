#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class StackPushType : uint8_t {
    CALL,  // JSR / BSR / LBSR
    PHA,   // Push accumulator
    PHX,   // Push X
    PHY,   // Push Y
    PHP,   // Push processor flags
    PHZ,   // Push Z (65CE02)
    BRK,   // BRK / interrupt
};

const char* stackPushTypeName(StackPushType t);

struct StackEntry {
    StackPushType type;
    uint32_t pushedByPc;  // address of the instruction that caused the push
    uint32_t value;       // CALL: callee addr; PHA/PHX/...: register value at time of push
};

class StackTrace {
public:
    void push(StackPushType type, uint32_t pc, uint32_t value);
    void pop();
    void clear();

    int depth() const { return (int)m_entries.size(); }

    // Returns up to n entries most-recent-first (n=0 returns all).
    std::vector<StackEntry> recent(int n = 0) const;

private:
    std::vector<StackEntry> m_entries; // index 0 = oldest
};
