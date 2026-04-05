#include "json_machine_loader.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

int JsonMachineLoader::loadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return 0;
    std::ostringstream ss;
    ss << f.rdbuf();
    return loadString(ss.str());
}

int JsonMachineLoader::loadString(const std::string& json) {
    try {
        auto doc = nlohmann::json::parse(json);
        return registerAll(doc);
    } catch (const nlohmann::json::exception&) {
        return 0;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int JsonMachineLoader::registerAll(const nlohmann::json& doc) {
    // The document may be a single machine object or an array of them.
    const nlohmann::json* machines = nullptr;
    nlohmann::json singleArr;

    if (doc.is_array()) {
        machines = &doc;
    } else if (doc.is_object()) {
        if (doc.contains("machines") && doc["machines"].is_array()) {
            machines = &doc["machines"];
        } else {
            // Treat the whole object as a single machine spec
            singleArr = nlohmann::json::array({ doc });
            machines  = &singleArr;
        }
    } else {
        return 0;
    }

    int count = 0;
    for (const auto& spec : *machines) {
        if (!spec.contains("id") || !spec["id"].is_string()) continue;
        std::string id = spec["id"].get<std::string>();

        // Capture spec by value for the lambda; buildFromSpec is Phase 1 stub.
        nlohmann::json specCopy = spec;
        MachineRegistry::instance().registerMachine(id,
            [this, specCopy]() -> MachineDescriptor* {
                return buildFromSpec(specCopy);
            });
        ++count;
    }
    return count;
}

MachineDescriptor* JsonMachineLoader::buildFromSpec(const nlohmann::json& /*spec*/) {
    // Phase 1 stub — full construction implemented in later phases.
    return nullptr;
}
