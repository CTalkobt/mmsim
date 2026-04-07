#include "kbd_c64.h"

// Key map: key name → {col (PA bit index), row (PB bit index)}
std::map<std::string, std::pair<int,int>> KbdC64::s_keyMap = {
    {"delete",{0,0}},    {"return",{0,1}},   {"crsr_right",{0,2}},{"f7",{0,3}},
    {"f1",{0,4}},        {"f3",{0,5}},       {"f5",{0,6}},        {"crsr_down",{0,7}},
    {"3",{1,0}},         {"w",{1,1}},        {"a",{1,2}},         {"4",{1,3}},
    {"z",{1,4}},         {"s",{1,5}},        {"e",{1,6}},         {"left_shift",{1,7}},
    {"5",{2,0}},         {"r",{2,1}},        {"d",{2,2}},         {"6",{2,3}},
    {"c",{2,4}},         {"f",{2,5}},        {"t",{2,6}},         {"x",{2,7}},
    {"7",{3,0}},         {"y",{3,1}},        {"g",{3,2}},         {"8",{3,3}},
    {"b",{3,4}},         {"h",{3,5}},        {"u",{3,6}},         {"v",{3,7}},
    {"9",{4,0}},         {"i",{4,1}},        {"j",{4,2}},         {"0",{4,3}},
    {"m",{4,4}},         {"k",{4,5}},        {"o",{4,6}},         {"n",{4,7}},
    {"plus",{5,0}},      {"p",{5,1}},        {"l",{5,2}},         {"minus",{5,3}},
    {"period",{5,4}},    {"colon",{5,5}},    {"at",{5,6}},        {"comma",{5,7}},
    {"pound",{6,0}},     {"asterisk",{6,1}}, {"semicolon",{6,2}}, {"home",{6,3}},
    {"right_shift",{6,4}},{"equal",{6,5}},   {"arrow_up",{6,6}},  {"slash",{6,7}},
    {"1",{7,0}},         {"arrow_left",{7,1}},{"ctrl",{7,2}},     {"2",{7,3}},
    {"space",{7,4}},     {"cbm",{7,5}},      {"q",{7,6}},         {"run_stop",{7,7}},
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
