#include "icore.h"
#include <cstring>

uint32_t ICore::regReadByName(const char* name) const {
    int idx = regIndexByName(name);
    if (idx >= 0) return regRead(idx);
    return 0;
}

void ICore::regWriteByName(const char* name, uint32_t val) {
    int idx = regIndexByName(name);
    if (idx >= 0) regWrite(idx, val);
}

int ICore::regIndexByName(const char* name) const {
    int count = regCount();
    for (int i = 0; i < count; ++i) {
        const RegDescriptor* d = regDescriptor(i);
        if (std::strcmp(d->name, name) == 0) return i;
        if (d->alias && std::strcmp(d->alias, name) == 0) return i;
    }
    return -1;
}
