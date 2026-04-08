#include "kbd_vic20.h"
#include <cstring>
#include <algorithm>

KbdVic20::KbdVic20()
    : m_colPort(this), m_rowPort(this)
{
    m_keyMap = {
        // col 0 (PB0)
        {"1",          {0, 0}}, {"3",          {0, 1}}, {"5",        {0, 2}}, {"7",     {0, 3}},
        {"9",          {0, 4}}, {"plus",       {0, 5}}, {"pound",    {0, 6}}, {"delete",{0, 7}},
        // col 1 (PB1)
        {"arrow_left", {1, 0}}, {"w",          {1, 1}}, {"r",        {1, 2}}, {"y",     {1, 3}},
        {"i",          {1, 4}}, {"p",          {1, 5}}, {"asterisk", {1, 6}}, {"return",{1, 7}},
        // col 2 (PB2)
        {"control",    {2, 0}}, {"a",          {2, 1}}, {"d",        {2, 2}}, {"g",     {2, 3}},
        {"j",          {2, 4}}, {"l",          {2, 5}}, {"semicolon",{2, 6}}, {"right", {2, 7}},
        // col 3 (PB3)
        {"run_stop",   {3, 0}}, {"left_shift", {3, 1}}, {"x",        {3, 2}}, {"v",     {3, 3}},
        {"n",          {3, 4}}, {"comma",      {3, 5}}, {"slash",    {3, 6}}, {"down",  {3, 7}},
        // col 4 (PB4)
        {"space",      {4, 0}}, {"z",          {4, 1}}, {"c",        {4, 2}}, {"b",     {4, 3}},
        {"m",          {4, 4}}, {"period",     {4, 5}}, {"right_shift",{4,6}},{"f1",    {4, 7}},
        // col 5 (PB5)
        {"cbm",        {5, 0}}, {"s",          {5, 1}}, {"f",        {5, 2}}, {"h",     {5, 3}},
        {"k",          {5, 4}}, {"minus",      {5, 5}}, {"equal",    {5, 6}}, {"f3",    {5, 7}},
        // col 6 (PB6)
        {"q",          {6, 0}}, {"e",          {6, 1}}, {"t",        {6, 2}}, {"u",     {6, 3}},
        {"o",          {6, 4}}, {"at",         {6, 5}}, {"up",       {6, 6}}, {"f5",    {6, 7}},
        // col 7 (PB7)
        {"2",          {7, 0}}, {"4",          {7, 1}}, {"6",        {7, 2}}, {"8",     {7, 3}},
        {"0",          {7, 4}}, {"arrow_up",   {7, 5}}, {"home",     {7, 6}}, {"f7",    {7, 7}},
    };
    clearKeys();
}

void KbdVic20::clearKeys() {
    std::memset(m_matrix, 0xFF, sizeof(m_matrix));
    updateMatrix();
}

// Interface uses (row, col) but for VIC-20 the first arg selects the PB
// column and the second is the PA row bit — consistent with the key map.
void KbdVic20::keyDown(int row, int col) {
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        m_matrix[row] &= ~(1 << col);
        updateMatrix();
    }
}

void KbdVic20::keyUp(int row, int col) {
    if (row >= 0 && row < 8 && col >= 0 && col < 8) {
        m_matrix[row] |= (1 << col);
        updateMatrix();
    }
}

bool KbdVic20::pressKeyByName(const std::string& keyName, bool down) {
    std::string lower = keyName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Handle symbolic names that require Shift on VIC-20
    if (lower == "exclaim")   return pressCombo({"left_shift", "1"}, down);
    if (lower == "dquote")    return pressCombo({"left_shift", "2"}, down);
    if (lower == "hash")      return pressCombo({"left_shift", "3"}, down);
    if (lower == "dollar")    return pressCombo({"left_shift", "4"}, down);
    if (lower == "percent")   return pressCombo({"left_shift", "5"}, down);
    if (lower == "ampersand") return pressCombo({"left_shift", "6"}, down);
    if (lower == "squote")    return pressCombo({"left_shift", "7"}, down);
    if (lower == "lparen")    return pressCombo({"left_shift", "8"}, down);
    if (lower == "rparen")    return pressCombo({"left_shift", "9"}, down);
    if (lower == "question")  return pressCombo({"left_shift", "slash"}, down);
    if (lower == "less")      return pressCombo({"left_shift", "comma"}, down);
    if (lower == "greater")   return pressCombo({"left_shift", "period"}, down);
    if (lower == "colon")     return pressCombo({"left_shift", "semicolon"}, down);
    if (lower == "bracket_left")  return pressCombo({"left_shift", "at"}, down); // Map to shifted @ for now?
    // Wait, on VIC-20 [ is Shift + : ? No.
    // I'll just use common host-side names for now.

    // Aliases
    if (lower == "uparrow")   lower = "arrow_up";
    if (lower == "leftarrow") lower = "arrow_left";

    if (m_keyMap.count(lower)) {
        auto pos = m_keyMap[lower];
        if (down) keyDown(pos.first, pos.second);
        else keyUp(pos.first, pos.second);
        return true;
    }
    return false;
}

bool KbdVic20::pressCombo(const std::vector<std::string>& keys, bool down) {
    bool ok = true;
    for (const auto& k : keys) {
        // Don't release modifiers in a combo; let the host send separate release events.
        if (!down && (k == "left_shift" || k == "right_shift" || k == "ctrl" || k == "cbm")) {
            continue;
        }
        if (m_keyMap.count(k)) {
            auto pos = m_keyMap[k];
            if (down) keyDown(pos.first, pos.second);
            else keyUp(pos.first, pos.second);
        } else {
            ok = false;
        }
    }
    return ok;
}

void KbdVic20::updateMatrix() {
    uint8_t cols = m_colPort.m_val;
    uint8_t rows = 0xFF;

    for (int i = 0; i < 8; ++i) {
        if (!(cols & (1 << i))) {
            rows &= m_matrix[i];
        }
    }
    m_rowVal = rows;
}
