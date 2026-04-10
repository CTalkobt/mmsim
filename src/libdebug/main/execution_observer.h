#pragma once

#include <cstdint>
#include <vector>
#include "libcore/main/icore.h"
#include "libmem/main/ibus.h"
#include "libtoolchain/main/idisasm.h"

/**
 * Interface for observing CPU execution and memory events.
 */
class ExecutionObserver {
public:
    virtual ~ExecutionObserver() {}

    /**
     * Called before each instruction executes.
     * Returns false to abort execution of the current instruction (e.g. breakpoint hit).
     */
    virtual bool onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) {
        (void)cpu; (void)bus; (void)entry;
        return true;
    }

    /**
     * Called whenever a byte is written to memory.
     */
    virtual void onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) {
        (void)bus; (void)addr; (void)before; (void)after;
    }

    /**
     * Called whenever a byte is read from memory.
     */
    virtual void onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) {
        (void)bus; (void)addr; (void)val;
    }
};

/**
 * A chain of execution observers.
 */
class ExecutionObserverChain : public ExecutionObserver {
public:
    void addObserver(ExecutionObserver* obs) {
        m_observers.push_back(obs);
    }

    void removeObserver(ExecutionObserver* obs) {
        for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
            if (*it == obs) {
                m_observers.erase(it);
                break;
            }
        }
    }

    bool onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) override {
        bool cont = true;
        for (auto* obs : m_observers) {
            if (!obs->onStep(cpu, bus, entry)) cont = false;
        }
        return cont;
    }

    void onMemoryWrite(IBus* bus, uint32_t addr, uint8_t before, uint8_t after) override {
        for (auto* obs : m_observers) {
            obs->onMemoryWrite(bus, addr, before, after);
        }
    }

    void onMemoryRead(IBus* bus, uint32_t addr, uint8_t val) override {
        for (auto* obs : m_observers) {
            obs->onMemoryRead(bus, addr, val);
        }
    }

private:
    std::vector<ExecutionObserver*> m_observers;
};
