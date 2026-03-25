#pragma once

#include "idisasm.h"
#include "libcore/main/icore.h"

class BaseDisassembler : public IDisassembler {
public:
    BaseDisassembler(ICore* cpu) : m_cpu(cpu) {}

    const char* isaName() const override { return m_cpu->isaName(); }

    int disasmOne(IBus* bus, uint32_t addr, char* buf, int bufsz) override {
        return m_cpu->disassembleOne(bus, addr, buf, bufsz);
    }

    int disasmEntry(IBus* bus, uint32_t addr, DisasmEntry& entry) override {
        return m_cpu->disassembleEntry(bus, addr, &entry);
    }

private:
    ICore* m_cpu;
};
