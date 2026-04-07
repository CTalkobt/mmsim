#include "kbd_c64.h"

// Key map: key name → {col (PA bit index), row (PB bit index)}
std::map<std::string, std::pair<int,int>> KbdC64::s_keyMap = {
    {"delete",{0,0}},    {"3",{0,1}},       {"5",{0,2}},        {"7",{0,3}},
    {"9",{0,4}},         {"plus",{0,5}},     {"pound",{0,6}},    {"1",{0,7}},
    {"return",{1,0}},    {"w",{1,1}},        {"r",{1,2}},        {"y",{1,3}},
    {"i",{1,4}},         {"p",{1,5}},        {"asterisk",{1,6}}, {"arrow_left",{1,7}},
    {"crsr_right",{2,0}},{"a",{2,1}},        {"d",{2,2}},        {"g",{2,3}},
    {"j",{2,4}},         {"l",{2,5}},        {"semicolon",{2,6}},{"ctrl",{2,7}},
    {"f7",{3,0}},        {"4",{3,1}},        {"6",{3,2}},        {"8",{3,3}},
    {"0",{3,4}},         {"minus",{3,5}},    {"home",{3,6}},     {"2",{3,7}},
    {"f1",{4,0}},        {"z",{4,1}},        {"c",{4,2}},        {"b",{4,3}},
    {"m",{4,4}},         {"period",{4,5}},   {"right_shift",{4,6}},{"space",{4,7}},
    {"f3",{5,0}},        {"s",{5,1}},        {"f",{5,2}},        {"h",{5,3}},
    {"k",{5,4}},         {"colon",{5,5}},    {"equal",{5,6}},    {"cbm",{5,7}},
    {"f5",{6,0}},        {"e",{6,1}},        {"t",{6,2}},        {"u",{6,3}},
    {"o",{6,4}},         {"at",{6,5}},       {"arrow_up",{6,6}}, {"q",{6,7}},
    {"crsr_down",{7,0}}, {"left_shift",{7,1}},{"x",{7,2}},       {"v",{7,3}},
    {"n",{7,4}},         {"comma",{7,5}},    {"slash",{7,6}},    {"run_stop",{7,7}},
};

void KbdC64::updateMatrix() {
    uint8_t rows = 0xFF;
    for (int col = 0; col < 8; ++col)
        if (!(m_colPort.m_val & (1 << col)))
            rows &= m_matrix[col];
    m_rowVal = rows;
}

bool KbdC64::pressCombo(const std::vector<std::string>& keys, bool down) {
    bool ok = true;
    for (const auto& k : keys) {
        if (!down && (k == "left_shift" || k == "right_shift" || k == "ctrl" || k == "cbm"))
            continue;
        auto it = s_keyMap.find(k);
        if (it != s_keyMap.end()) {
            int col = it->second.first;
            int row = it->second.second;
            if (down) m_matrix[col] &= ~(uint8_t)(1 << row);
            else      m_matrix[col] |=  (uint8_t)(1 << row);
        } else {
            ok = false;
        }
    }
    updateMatrix();
    return ok;
}

bool KbdC64::pressKeyByName(const std::string& keyName, bool down) {
    std::string lower = keyName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "exclaim")       return pressCombo({"left_shift", "1"}, down);
    if (lower == "dquote")        return pressCombo({"left_shift", "2"}, down);
    if (lower == "hash")          return pressCombo({"left_shift", "3"}, down);
    if (lower == "dollar")        return pressCombo({"left_shift", "4"}, down);
    if (lower == "percent")       return pressCombo({"left_shift", "5"}, down);
    if (lower == "ampersand")     return pressCombo({"left_shift", "6"}, down);
    if (lower == "squote")        return pressCombo({"left_shift", "7"}, down);
    if (lower == "lparen")        return pressCombo({"left_shift", "8"}, down);
    if (lower == "rparen")        return pressCombo({"left_shift", "9"}, down);
    if (lower == "question")      return pressCombo({"left_shift", "slash"}, down);
    if (lower == "less")          return pressCombo({"left_shift", "comma"}, down);
    if (lower == "greater")       return pressCombo({"left_shift", "period"}, down);
    if (lower == "bracket_left")  return pressCombo({"left_shift", "colon"}, down);
    if (lower == "bracket_right") return pressCombo({"left_shift", "semicolon"}, down);

    if (lower == "uparrow")   lower = "arrow_up";
    if (lower == "leftarrow") lower = "arrow_left";
    if (lower == "right")     lower = "crsr_right";
    if (lower == "down")      lower = "crsr_down";
    if (lower == "up")        return pressCombo({"left_shift", "crsr_down"}, down);
    if (lower == "left")      return pressCombo({"left_shift", "crsr_right"}, down);

    auto it = s_keyMap.find(lower);
    if (it != s_keyMap.end()) {
        int col = it->second.first;
        int row = it->second.second;
        if (down) m_matrix[col] &= ~(uint8_t)(1 << row);
        else      m_matrix[col] |=  (uint8_t)(1 << row);
        updateMatrix();
        return true;
    }
    return false;
}
