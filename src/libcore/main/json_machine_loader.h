#pragma once

#include <string>
#include <nlohmann/json.hpp>

/**
 * Loads machine definitions from JSON files and registers them with
 * MachineRegistry so they appear alongside plugin-registered machines.
 *
 * Phase 1: skeleton only — loadFile() parses and validates JSON structure;
 * buildFromSpec() returns nullptr until later phases implement construction.
 */
class JsonMachineLoader {
public:
    /**
     * Parse a JSON machine-definition file and register all machines it
     * contains with MachineRegistry::instance().
     *
     * @param path   Filesystem path to a .json file.
     * @return       Number of machine IDs successfully registered (0 on error).
     */
    int loadFile(const std::string& path);

    /**
     * Parse a JSON machine-definition string (for testing) and register all
     * machines it contains.
     *
     * @param json   JSON text.
     * @return       Number of machine IDs successfully registered (0 on error).
     */
    int loadString(const std::string& json);

private:
    /**
     * Register all machine entries found in an already-parsed document.
     * Returns the number of IDs registered.
     */
    int registerAll(const nlohmann::json& doc);

    /**
     * Build a MachineDescriptor from a single machine spec node.
     * Currently returns nullptr (Phase 1 stub).
     */
    struct MachineDescriptor* buildFromSpec(const nlohmann::json& spec);
};
