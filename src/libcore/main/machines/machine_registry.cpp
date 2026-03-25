#include "machine_registry.h"

static MachineRegistry* s_instance = nullptr;

MachineRegistry& MachineRegistry::instance() {
    if (!s_instance) s_instance = new MachineRegistry();
    return *s_instance;
}

void MachineRegistry::setInstance(MachineRegistry* inst) {
    s_instance = inst;
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
