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

void MachineRegistry::registerMachine(const std::string& id, FactoryFn factory, const std::string& description) {
    m_factories[id] = factory;
    m_descriptions[id] = description;
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

void MachineRegistry::enumerateDetailed(std::vector<std::pair<std::string, std::string>>& out) {
    for (const auto& pair : m_factories) {
        auto it = m_descriptions.find(pair.first);
        std::string desc = (it != m_descriptions.end()) ? it->second : "";
        out.push_back({pair.first, desc});
    }
}
