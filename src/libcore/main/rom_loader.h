#pragma once

#include <cstdint>
#include <cstddef>

/**
 * Loads a binary ROM file into a buffer.
 * 
 * @param path The filesystem path to the ROM file.
 * @param buf The destination buffer to load the ROM into.
 * @param expectedSize The expected size of the ROM in bytes.
 * @return true if the ROM was loaded successfully and matched the expected size, false otherwise.
 */
bool romLoad(const char *path, uint8_t *buf, size_t expectedSize);
