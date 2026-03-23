#pragma once

#include "libcore/machine_desc.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

/**
 * Global registry for machines.
 */
class MachineRegistry {
public:
    using FactoryFn = std::function<MachineDescriptor*()>;

    static MachineRegistry& instance();

    void registerMachine(const std::string& id, FactoryFn factory);
    MachineDescriptor* createMachine(const std::string& id);
    void enumerate(std::vector<std::string>& ids);

private:
    MachineRegistry() = default;
    std::map<std::string, FactoryFn> m_factories;
};
