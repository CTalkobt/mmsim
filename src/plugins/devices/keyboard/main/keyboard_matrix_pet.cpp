#include "keyboard_matrix_pet.h"
#include "include/mmemu_plugin_api.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

PetKeyboardMatrix::PetKeyboardMatrix(Layout layout) : m_layout(layout) {
    std::memset(m_matrix, 0xFF, sizeof(m_matrix));
    initKeyMap();
}

void PetKeyboardMatrix::initKeyMap() {
    m_keyMap.clear();

    if (m_layout == Layout::GRAPHICS) {
        // PET Graphics (N) Keyboard Matrix
        // Row 0
        m_keyMap["!"]           = {0, 0x80}; m_keyMap["1"] = {0, 0x80};
        m_keyMap["#"]           = {0, 0x40}; m_keyMap["3"] = {0, 0x40};
        m_keyMap["%"]           = {0, 0x20}; m_keyMap["5"] = {0, 0x20};
        m_keyMap["&"]           = {0, 0x10}; m_keyMap["7"] = {0, 0x10};
        m_keyMap["("]           = {0, 0x08}; m_keyMap["9"] = {0, 0x08};
        m_keyMap["leftarrow"]   = {0, 0x04};
        m_keyMap["home"]        = {0, 0x02};
        m_keyMap["kp7"]         = {0, 0x01};

        // Row 1
        m_keyMap["\""]          = {1, 0x80}; m_keyMap["2"] = {1, 0x80};
        m_keyMap["$"]           = {1, 0x40}; m_keyMap["4"] = {1, 0x40};
        m_keyMap["'"]           = {1, 0x20}; m_keyMap["6"] = {1, 0x20};
        m_keyMap["("]           = {1, 0x10}; m_keyMap["8"] = {1, 0x10};
        m_keyMap[")"]           = {1, 0x08}; m_keyMap["0"] = {1, 0x08};
        m_keyMap["delete"]      = {1, 0x04};
        m_keyMap["up"]          = {1, 0x02};
        m_keyMap["kp8"]         = {1, 0x01};

        // Row 2
        m_keyMap["q"]           = {2, 0x80};
        m_keyMap["e"]           = {2, 0x40};
        m_keyMap["t"]           = {2, 0x20};
        m_keyMap["u"]           = {2, 0x10};
        m_keyMap["o"]           = {2, 0x08};
        m_keyMap["uparrow"]     = {2, 0x04};
        m_keyMap["7"]           = {2, 0x02};
        m_keyMap["kp9"]         = {2, 0x01};

        // Row 3
        m_keyMap["w"]           = {3, 0x80};
        m_keyMap["r"]           = {3, 0x40};
        m_keyMap["y"]           = {3, 0x20};
        m_keyMap["i"]           = {3, 0x10};
        m_keyMap["p"]           = {3, 0x08};
        m_keyMap["return"]      = {3, 0x04};
        m_keyMap["8"]           = {3, 0x02};
        m_keyMap["kp4"]         = {3, 0x01};

        // Row 4
        m_keyMap["a"]           = {4, 0x80};
        m_keyMap["d"]           = {4, 0x40};
        m_keyMap["g"]           = {4, 0x20};
        m_keyMap["j"]           = {4, 0x10};
        m_keyMap["l"]           = {4, 0x08};
        m_keyMap["right"]       = {4, 0x04};
        m_keyMap["4"]           = {4, 0x02};
        m_keyMap["kp5"]         = {4, 0x01};

        // Row 5
        m_keyMap["s"]           = {5, 0x80};
        m_keyMap["f"]           = {5, 0x40};
        m_keyMap["h"]           = {5, 0x20};
        m_keyMap["k"]           = {5, 0x10};
        m_keyMap["colon"]       = {5, 0x08};
        m_keyMap["5"]           = {5, 0x04};
        m_keyMap["6"]           = {5, 0x02};
        m_keyMap["kp6"]         = {5, 0x01};

        // Row 6
        m_keyMap["z"]           = {6, 0x80};
        m_keyMap["c"]           = {6, 0x40};
        m_keyMap["b"]           = {6, 0x20};
        m_keyMap["m"]           = {6, 0x10};
        m_keyMap["semicolon"]   = {6, 0x08};
        m_keyMap["down"]        = {6, 0x04};
        m_keyMap["1"]           = {6, 0x02};
        m_keyMap["kp1"]         = {6, 0x01};

        // Row 7
        m_keyMap["x"]           = {7, 0x80};
        m_keyMap["v"]           = {7, 0x40};
        m_keyMap["n"]           = {7, 0x20};
        m_keyMap["comma"]       = {7, 0x10};
        m_keyMap["slash"]       = {7, 0x08};
        m_keyMap["2"]           = {7, 0x04};
        m_keyMap["3"]           = {7, 0x02};
        m_keyMap["kp2"]         = {7, 0x01};

        // Row 8
        m_keyMap["left_shift"]  = {8, 0x80};
        m_keyMap["right_shift"] = {8, 0x08};
        m_keyMap["at"]          = {8, 0x40};
        m_keyMap["bracket_right"] = {8, 0x20};
        m_keyMap["kp3"]         = {8, 0x01};

        // Row 9
        m_keyMap["rvs"]         = {9, 0x80};
        m_keyMap["bracket_left"] = {9, 0x40};
        m_keyMap["run_stop"]    = {9, 0x08};
        m_keyMap["space"]       = {9, 0x04};
        m_keyMap["minus"]       = {9, 0x02};
        m_keyMap["kp0"]         = {9, 0x01};

    } else {
        // PET Business (B) Keyboard Matrix
        // Row 0
        m_keyMap["2"]           = {0, 0x80};
        m_keyMap["5"]           = {0, 0x40};
        m_keyMap["8"]           = {0, 0x20};
        m_keyMap["minus"]       = {0, 0x10};
        m_keyMap["kp8"]         = {0, 0x08};
        m_keyMap["kp5"]         = {0, 0x04};
        m_keyMap["kp2"]         = {0, 0x02};
        m_keyMap["kp0"]         = {0, 0x01};

        // Row 1
        m_keyMap["1"]           = {1, 0x80};
        m_keyMap["4"]           = {1, 0x40};
        m_keyMap["7"]           = {1, 0x20};
        m_keyMap["0"]           = {1, 0x10};
        m_keyMap["kp7"]         = {1, 0x08};
        m_keyMap["kp4"]         = {1, 0x04};
        m_keyMap["kp1"]         = {1, 0x02};

        // Row 2
        m_keyMap["run_stop"]    = {2, 0x80};
        m_keyMap["s"]           = {2, 0x40};
        m_keyMap["f"]           = {2, 0x20};
        m_keyMap["h"]           = {2, 0x10};
        m_keyMap["bracket_right"] = {2, 0x08};
        m_keyMap["k"]           = {2, 0x04};
        m_keyMap["colon"]       = {2, 0x02};

        // Row 3
        m_keyMap["a"]           = {3, 0x80};
        m_keyMap["d"]           = {3, 0x40};
        m_keyMap["g"]           = {3, 0x20};
        m_keyMap["j"]           = {3, 0x10};
        m_keyMap["l"]           = {3, 0x08};
        m_keyMap["semicolon"]   = {3, 0x04};
        m_keyMap["return"]      = {3, 0x02};
        m_keyMap["kp9"]         = {3, 0x01};

        // Row 4
        m_keyMap["w"]           = {4, 0x80};
        m_keyMap["r"]           = {4, 0x40};
        m_keyMap["y"]           = {4, 0x20};
        m_keyMap["i"]           = {4, 0x10};
        m_keyMap["p"]           = {4, 0x08};
        m_keyMap["delete"]      = {4, 0x04};
        m_keyMap["kp6"]         = {4, 0x02};
        m_keyMap["kp3"]         = {4, 0x01};

        // Row 5
        m_keyMap["q"]           = {5, 0x80};
        m_keyMap["e"]           = {5, 0x40};
        m_keyMap["t"]           = {5, 0x20};
        m_keyMap["u"]           = {5, 0x10};
        m_keyMap["o"]           = {5, 0x08};

        // Row 6
        m_keyMap["left_shift"]  = {6, 0x80};
        m_keyMap["x"]           = {6, 0x40};
        m_keyMap["v"]           = {6, 0x20};
        m_keyMap["n"]           = {6, 0x10};
        m_keyMap["comma"]       = {6, 0x08};
        m_keyMap["slash"]       = {6, 0x04};
        m_keyMap["right_shift"] = {6, 0x02};

        // Row 7
        m_keyMap["z"]           = {7, 0x80};
        m_keyMap["c"]           = {7, 0x40};
        m_keyMap["b"]           = {7, 0x20};
        m_keyMap["m"]           = {7, 0x10};
        m_keyMap["period"]      = {7, 0x08};
        m_keyMap["up"]          = {7, 0x04};
        m_keyMap["down"]        = {7, 0x02};

        // Row 8
        m_keyMap["rvs"]         = {8, 0x80};
        m_keyMap["space"]       = {8, 0x40};
        m_keyMap["left"]        = {8, 0x04};
        m_keyMap["right"]       = {8, 0x02};

        // Row 9
        m_keyMap["3"]           = {9, 0x80};
        m_keyMap["6"]           = {9, 0x40};
        m_keyMap["9"]           = {9, 0x20};
        m_keyMap["home"]        = {9, 0x04};
        m_keyMap["delete"]      = {9, 0x02};
        m_keyMap["run_stop"]    = {9, 0x01};
    }
}

void PetKeyboardMatrix::keyDown(const std::string& keyName) {
    if (m_logger && m_logNamed) {
        char buf[64]; snprintf(buf, sizeof(buf), "keyDown: %s", keyName.c_str());
        m_logNamed(m_logger, SIM_LOG_DEBUG, buf);
    }
    auto it = m_keyMap.find(keyName);
    if (it != m_keyMap.end()) {
        m_matrix[it->second.col] &= ~it->second.bit;
    }
}

void PetKeyboardMatrix::keyUp(const std::string& keyName) {
    if (m_logger && m_logNamed) {
        char buf[64]; snprintf(buf, sizeof(buf), "keyUp: %s", keyName.c_str());
        m_logNamed(m_logger, SIM_LOG_DEBUG, buf);
    }
    auto it = m_keyMap.find(keyName);
    if (it != m_keyMap.end()) {
        m_matrix[it->second.col] |= it->second.bit;
    }
}

uint8_t PetKeyboardMatrix::readPort() {
    uint8_t result = 0xFF;
    uint8_t rowSelect = m_selectedRow & 0x0F;
    
    // In PET, the 74145 decoder selects a single row (0-9).
    // If the value is 10-15, no row is selected.
    if (rowSelect < 10) {
        result &= m_matrix[rowSelect];
    }
    
    return result;
}

void PetKeyboardMatrix::writePort(uint8_t val) {
    m_selectedRow = val;
}

void PetKeyboardMatrix::setDdr(uint8_t ddr) {
    // DDR handled by PIA
}
