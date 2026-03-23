#include "core_registry.h"

CoreRegistry& CoreRegistry::instance() {
    static CoreRegistry s_instance;
    return s_instance;
}

void CoreRegistry::registerCore(const std::string& name, const std::string& variant, const std::string& license, FactoryFn factory) {
    m_cores[name] = {name, variant, license, factory};
}

ICore* CoreRegistry::createCore(const std::string& name) {
    if (m_cores.count(name)) {
        return m_cores[name].factory();
    }
    return nullptr;
}

void CoreRegistry::enumerate(std::vector<CoreInfo>& out) {
    for (const auto& pair : m_cores) {
        out.push_back(pair.second);
    }
}
