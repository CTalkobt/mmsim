#pragma once
#include <string>

/**
 * Maps PC key code (wxWidgets) to emulated key name for unshifted keys.
 */
std::string wxKeyToVic20Name(int code);
