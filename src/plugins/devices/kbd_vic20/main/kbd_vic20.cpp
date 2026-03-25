#include "kbd_vic20.h"
#include <cstring>
#include <algorithm>

std::map<std::string, std::pair<int, int>> KbdVic20::s_keyMap = {
    {"1", {0, 0}}, {"3", {0, 1}}, {"5", {0, 2}}, {"7", {0, 3}}, {"9", {0, 4}}, {"0", {0, 5}}, {"pound", {0, 6}}, {"f1", {0, 7}},
    {"control", {1, 0}}, {"w", {1, 1}}, {"r", {1, 2}}, {"y", {1, 3}}, {"i", {1, 4}}, {"p", {1, 5}}, {"at", {1, 6}}, {"f3", {1, 7}},
    {"left_shift", {2, 0}}, {"a", {2, 1}}, {"d", {2, 2}}, {"g", {2, 3}}, {"j", {2, 4}}, {"l", {2, 5}}, {"semicolon", {2, 6}}, {"f5", {2, 7}},
    {"z", {3, 0}}, {"c", {3, 1}}, {"b", {3, 2}}, {"m", {3, 3}}, {"period", {3, 4}}, {"right_shift", {3, 5}}, {"equal", {3, 6}}, {"f7", {3, 7}},
    {"space", {4, 0}}, {"s", {4, 1}}, {"f", {4, 2}}, {"h", {4, 3}}, {"k", {4, 4}}, {"comma", {4, 5}}, {"slash", {4, 6}}, {"down", {4, 7}},
    {"q", {5, 0}}, {"e", {5, 1}}, {"t", {5, 2}}, {"u", {5, 3}}, {"o", {5, 4}}, {"bracket_left", {5, 5}}, {"return", {5, 6}}, {"up", {5, 7}},
    {"2", {6, 0}}, {"4", {6, 1}}, {"6", {6, 2}}, {"8", {6, 3}}, {"0", {6, 4}}, {"minus", {6, 5}}, {"home", {6, 6}}, {"left", {6, 7}},
    {"arrow_left", {7, 0}}, {"x", {7, 1}}, {"v", {7, 2}}, {"n", {7, 3}}, {"m", {7, 4}}, {"bracket_right", {7, 5}}, {"clear", {7, 6}}, {"right", {7, 7}}
};

KbdVic20::KbdVic20()
    : m_colPort(this), m_rowPort(this)
{
    clearKeys();
}

void KbdVic20::clearKeys() {
    std::memset(m_matrix, 0xFF, sizeof(m_matrix));
    updateMatrix();
}

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
    
    if (s_keyMap.count(lower)) {
        auto pos = s_keyMap[lower];
        if (down) keyDown(pos.first, pos.second);
        else keyUp(pos.first, pos.second);
        return true;
    }
    return false;
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
