#include "device_registry.h"

DeviceRegistry& DeviceRegistry::instance() {
    static DeviceRegistry s_instance;
    return s_instance;
}

void DeviceRegistry::registerDevice(const std::string& name, FactoryFn factory) {
    m_factories[name] = factory;
}

IOHandler* DeviceRegistry::createDevice(const std::string& typeName) {
    auto it = m_factories.find(typeName);
    if (it != m_factories.end()) {
        return it->second();
    }
    return nullptr;
}

void DeviceRegistry::enumerate(std::vector<std::string>& out) {
    for (const auto& kv : m_factories) {
        out.push_back(kv.first);
    }
}
