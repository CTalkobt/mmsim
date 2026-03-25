#include "rom_loader.h"
#include <cstdio>

bool romLoad(const char *path, uint8_t *buf, size_t expectedSize) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open ROM file: %s\n", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t actualSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (actualSize != expectedSize) {
        fprintf(stderr, "Error: ROM file size mismatch: %s (expected %zu, got %zu)\n",
                path, expectedSize, actualSize);
        fclose(f);
        return false;
    }

    size_t bytesRead = fread(buf, 1, expectedSize, f);
    fclose(f);

    if (bytesRead != expectedSize) {
        fprintf(stderr, "Error: Could not read full ROM file: %s\n", path);
        return false;
    }

    return true;
}
