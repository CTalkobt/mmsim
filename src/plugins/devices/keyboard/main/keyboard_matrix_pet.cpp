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
        // PET 2001N Graphics keyboard matrix derived from BASIC 1 Editor ROM decode table.
        // Port A bits 0-3 select rows 0-9 via 74145 decoder.
        // Port B bit 7 = column 0 (first table entry), bit 0 = column 7 (last entry).
        // ROM table stored rows 9..0 (highest first); decode confirmed against ROM bytes.

        // Row 0 (Port A = 0xF0): cursor-right, HOME, ←char, (, &, %, #, !
        m_keyMap["right"]         = {0, 0x80};  // cursor right (0x1D)
        m_keyMap["home"]          = {0, 0x40};  // HOME/CLR   (0x13)
        m_keyMap["leftarrow"]     = {0, 0x20};  // ← graphical char (0x5F)
        m_keyMap["lparen"]        = {0, 0x10};  // (
        m_keyMap["ampersand"]     = {0, 0x08};  // &
        m_keyMap["percent"]       = {0, 0x04};  // %
        m_keyMap["hash"]          = {0, 0x02};  // #
        m_keyMap["exclaim"]       = {0, 0x01};  // !

        // Row 1 (Port A = 0xF1): DEL, cursor-down, (no key), ), £char, ', $, "
        m_keyMap["delete"]        = {1, 0x80};  // DEL/backspace (0x14)
        m_keyMap["down"]          = {1, 0x40};  // cursor down   (0x11)
        // {1, 0x20} = no key (0xFF)
        m_keyMap["rparen"]        = {1, 0x10};  // )
        // {1, 0x08} = £ graphical char (0x5C) — no PC equivalent
        m_keyMap["squote"]        = {1, 0x04};  // ' (single quote)
        m_keyMap["dollar"]        = {1, 0x02};  // $
        m_keyMap["dquote"]        = {1, 0x01};  // "

        // Row 2 (Port A = 0xF2): 9, 7, ^uparrow, O, U, T, E, Q
        m_keyMap["9"]             = {2, 0x80};
        m_keyMap["7"]             = {2, 0x40};
        m_keyMap["uparrow"]       = {2, 0x20};  // ↑ character key (0x5E)
        m_keyMap["o"]             = {2, 0x10};
        m_keyMap["u"]             = {2, 0x08};
        m_keyMap["t"]             = {2, 0x04};
        m_keyMap["e"]             = {2, 0x02};
        m_keyMap["q"]             = {2, 0x01};

        // Row 3 (Port A = 0xF3): /, 8, (no key), P, I, Y, R, W
        m_keyMap["slash"]         = {3, 0x80};
        m_keyMap["8"]             = {3, 0x40};
        // {3, 0x20} = no key
        m_keyMap["p"]             = {3, 0x10};
        m_keyMap["i"]             = {3, 0x08};
        m_keyMap["y"]             = {3, 0x04};
        m_keyMap["r"]             = {3, 0x02};
        m_keyMap["w"]             = {3, 0x01};

        // Row 4 (Port A = 0xF4): 6, 4, (no key), L, J, G, D, A
        m_keyMap["6"]             = {4, 0x80};
        m_keyMap["4"]             = {4, 0x40};
        // {4, 0x20} = no key
        m_keyMap["l"]             = {4, 0x10};
        m_keyMap["j"]             = {4, 0x08};
        m_keyMap["g"]             = {4, 0x04};
        m_keyMap["d"]             = {4, 0x02};
        m_keyMap["a"]             = {4, 0x01};

        // Row 5 (Port A = 0xF5): *, 5, (no key), :, K, H, F, S
        m_keyMap["asterisk"]      = {5, 0x80};  // * (dedicated key)
        m_keyMap["5"]             = {5, 0x40};
        // {5, 0x20} = no key
        m_keyMap["colon"]         = {5, 0x10};  // :
        m_keyMap["k"]             = {5, 0x08};
        m_keyMap["h"]             = {5, 0x04};
        m_keyMap["f"]             = {5, 0x02};
        m_keyMap["s"]             = {5, 0x01};

        // Row 6 (Port A = 0xF6): 3, 1, RETURN, ;, M, B, C, Z
        m_keyMap["3"]             = {6, 0x80};
        m_keyMap["1"]             = {6, 0x40};
        m_keyMap["return"]        = {6, 0x20};
        m_keyMap["semicolon"]     = {6, 0x10};  // ;
        m_keyMap["m"]             = {6, 0x08};
        m_keyMap["b"]             = {6, 0x04};
        m_keyMap["c"]             = {6, 0x02};
        m_keyMap["z"]             = {6, 0x01};

        // Row 7 (Port A = 0xF7): +, 2, (no key), ?, comma, N, V, X
        m_keyMap["plus"]          = {7, 0x80};  // + (dedicated key)
        m_keyMap["2"]             = {7, 0x40};
        // {7, 0x20} = no key
        m_keyMap["question"]      = {7, 0x10};  // ? (dedicated key)
        m_keyMap["comma"]         = {7, 0x08};
        m_keyMap["n"]             = {7, 0x04};
        m_keyMap["v"]             = {7, 0x02};
        m_keyMap["x"]             = {7, 0x01};

        // Row 8 (Port A = 0xF8): -, 0, LSHIFT, > (right cursor?), (no key), ], @, RSHIFT
        m_keyMap["minus"]         = {8, 0x80};
        m_keyMap["0"]             = {8, 0x40};
        m_keyMap["left_shift"]    = {8, 0x20};  // LEFT SHIFT (null char = modifier only)
        // {8, 0x10} = '>' (cursor right or graphical)
        // {8, 0x08} = no key
        m_keyMap["bracket_right"] = {8, 0x04};  // ]
        m_keyMap["at"]            = {8, 0x02};  // @
        m_keyMap["right_shift"]   = {8, 0x01};  // RIGHT SHIFT (null char = modifier only)

        // Row 9 (Port A = 0xF9): =, ., (no key), RUN/STOP, <, SPACE, [, RVS
        m_keyMap["kpeq"]          = {9, 0x80};  // = (keypad equals)
        m_keyMap["period"]        = {9, 0x40};
        // {9, 0x20} = no key
        m_keyMap["run_stop"]      = {9, 0x10};  // RUN/STOP
        m_keyMap["less"]          = {9, 0x08};  // < key
        m_keyMap["space"]         = {9, 0x04};
        m_keyMap["bracket_left"]  = {9, 0x02};  // [
        m_keyMap["rvs"]           = {9, 0x01};  // RVS ON

    } else {
        // PET Business (B) Keyboard Matrix — 4032 / 8032.
        // Derived from pet4032/editor.bin ROM decode table (offset 0x73F).
        // The Business keyboard uses the SAME matrix positions as the Graphics
        // keyboard for all shared keys; the ROM decode tables are identical.
        // The Business layout adds TAB and an extra key in row 9.
        // Numeric keypad keys share matrix positions with the main digit keys.

        // Row 0 (Port A = 0xF0): cursor-right, HOME, ←char, (, &, %, #, !
        m_keyMap["right"]         = {0, 0x80};
        m_keyMap["home"]          = {0, 0x40};
        m_keyMap["leftarrow"]     = {0, 0x20};  // ← graphical char (0x5F)
        m_keyMap["lparen"]        = {0, 0x10};  // (
        m_keyMap["ampersand"]     = {0, 0x08};  // &
        m_keyMap["percent"]       = {0, 0x04};  // %
        m_keyMap["hash"]          = {0, 0x02};  // #
        m_keyMap["exclaim"]       = {0, 0x01};  // !

        // Row 1 (Port A = 0xF1): DEL, cursor-down, TAB, ), £char, ', $, "
        m_keyMap["delete"]        = {1, 0x80};
        m_keyMap["down"]          = {1, 0x40};
        m_keyMap["tab"]           = {1, 0x20};  // TAB (Business-only, 0x09)
        m_keyMap["rparen"]        = {1, 0x10};  // )
        // {1, 0x08} = £ graphical char — no PC equivalent
        m_keyMap["squote"]        = {1, 0x04};  // '
        m_keyMap["dollar"]        = {1, 0x02};  // $
        m_keyMap["dquote"]        = {1, 0x01};  // "

        // Row 2 (Port A = 0xF2): 9, 7, ^uparrow, O, U, T, E, Q
        m_keyMap["9"]             = {2, 0x80};
        m_keyMap["7"]             = {2, 0x40};
        m_keyMap["uparrow"]       = {2, 0x20};  // ↑ char (0x5E)
        m_keyMap["o"]             = {2, 0x10};
        m_keyMap["u"]             = {2, 0x08};
        m_keyMap["t"]             = {2, 0x04};
        m_keyMap["e"]             = {2, 0x02};
        m_keyMap["q"]             = {2, 0x01};

        // Row 3 (Port A = 0xF3): /, 8, (no key), P, I, Y, R, W
        m_keyMap["slash"]         = {3, 0x80};
        m_keyMap["8"]             = {3, 0x40};
        m_keyMap["p"]             = {3, 0x10};
        m_keyMap["i"]             = {3, 0x08};
        m_keyMap["y"]             = {3, 0x04};
        m_keyMap["r"]             = {3, 0x02};
        m_keyMap["w"]             = {3, 0x01};

        // Row 4 (Port A = 0xF4): 6, 4, (no key), L, J, G, D, A
        m_keyMap["6"]             = {4, 0x80};
        m_keyMap["4"]             = {4, 0x40};
        m_keyMap["l"]             = {4, 0x10};
        m_keyMap["j"]             = {4, 0x08};
        m_keyMap["g"]             = {4, 0x04};
        m_keyMap["d"]             = {4, 0x02};
        m_keyMap["a"]             = {4, 0x01};

        // Row 5 (Port A = 0xF5): *, 5, (no key), :, K, H, F, S
        m_keyMap["asterisk"]      = {5, 0x80};
        m_keyMap["5"]             = {5, 0x40};
        m_keyMap["colon"]         = {5, 0x10};
        m_keyMap["k"]             = {5, 0x08};
        m_keyMap["h"]             = {5, 0x04};
        m_keyMap["f"]             = {5, 0x02};
        m_keyMap["s"]             = {5, 0x01};

        // Row 6 (Port A = 0xF6): 3, 1, RETURN, ;, M, B, C, Z
        m_keyMap["3"]             = {6, 0x80};
        m_keyMap["1"]             = {6, 0x40};
        m_keyMap["return"]        = {6, 0x20};
        m_keyMap["semicolon"]     = {6, 0x10};
        m_keyMap["m"]             = {6, 0x08};
        m_keyMap["b"]             = {6, 0x04};
        m_keyMap["c"]             = {6, 0x02};
        m_keyMap["z"]             = {6, 0x01};

        // Row 7 (Port A = 0xF7): +, 2, (no key), ?, comma, N, V, X
        m_keyMap["plus"]          = {7, 0x80};
        m_keyMap["2"]             = {7, 0x40};
        m_keyMap["question"]      = {7, 0x10};
        m_keyMap["comma"]         = {7, 0x08};
        m_keyMap["n"]             = {7, 0x04};
        m_keyMap["v"]             = {7, 0x02};
        m_keyMap["x"]             = {7, 0x01};

        // Row 8 (Port A = 0xF8): -, 0, LSHIFT, (no key), (no key), ], @, RSHIFT
        m_keyMap["minus"]         = {8, 0x80};
        m_keyMap["0"]             = {8, 0x40};
        m_keyMap["left_shift"]    = {8, 0x20};
        m_keyMap["bracket_right"] = {8, 0x04};
        m_keyMap["at"]            = {8, 0x02};
        m_keyMap["right_shift"]   = {8, 0x01};

        // Row 9 (Port A = 0xF9): =, ., (ctrl/stop), RUN/STOP, <, SPACE, [, RVS
        m_keyMap["kpeq"]          = {9, 0x80};
        m_keyMap["period"]        = {9, 0x40};
        // {9, 0x20} = 0x10 (DLE) on 4032 — extra control key, no PC mapping
        m_keyMap["run_stop"]      = {9, 0x10};
        m_keyMap["less"]          = {9, 0x08};
        m_keyMap["space"]         = {9, 0x04};
        m_keyMap["bracket_left"]  = {9, 0x02};
        m_keyMap["rvs"]           = {9, 0x01};
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
