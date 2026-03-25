#include "icore.h"
#include <cstring>

ICore::~ICore() {}

uint32_t ICore::regReadByName(const char* name) const {
    int idx = regIndexByName(name);
    return (idx >= 0) ? regRead(idx) : 0;
}

void ICore::regWriteByName(const char* name, uint32_t val) {
    int idx = regIndexByName(name);
    if (idx >= 0) regWrite(idx, val);
}

int ICore::regIndexByName(const char* name) const {
    int count = regCount();
    for (int i = 0; i < count; ++i) {
        const auto* desc = regDescriptor(i);
        if (desc && desc->name && std::strcmp(desc->name, name) == 0) return i;
        if (desc && desc->alias && std::strcmp(desc->alias, name) == 0) return i;
    }
    return -1;
}
