#include "machine_registry.h"

MachineRegistry& MachineRegistry::instance() {
    static MachineRegistry s_instance;
    return s_instance;
}

void MachineRegistry::registerMachine(const std::string& id, FactoryFn factory) {
    m_factories[id] = factory;
}

MachineDescriptor* MachineRegistry::createMachine(const std::string& id) {
    if (m_factories.count(id)) {
        return m_factories[id]();
    }
    return nullptr;
}

void MachineRegistry::enumerate(std::vector<std::string>& ids) {
    for (const auto& pair : m_factories) {
        ids.push_back(pair.first);
    }
}
