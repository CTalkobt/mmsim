#pragma once

#include <cstdint>

// mmemu public plugin ABI — version 1
#define MMEMU_PLUGIN_API_VERSION 1

class ICore;
class IDisassembler;
class IAssembler;
class IOHandler;
struct MachineDescriptor;

/**
 * Host services provided to plugins.
 */
struct SimPluginHostAPI {
    void (*log)(int level, const char* msg);
    // Future: IBus factory helpers, memory allocation, etc.
};

/**
 * Descriptor for a CPU core provided by a plugin.
 */
struct CorePluginInfo {
    const char* name;           // e.g. "6502", "Z80"
    const char* variant;        // e.g. "NMOS", "CMOS"
    const char* licenseClass;   // "open", "commercial"
    ICore* (*create)();
};

/**
 * Descriptor for a toolchain provided by a plugin.
 */
struct ToolchainPluginInfo {
    const char* isa;            // e.g. "6502"
    IDisassembler* (*createDisassembler)();
    IAssembler*    (*createAssembler)();
};

/**
 * Descriptor for an I/O device provided by a plugin.
 */
struct DevicePluginInfo {
    const char* name;
    IOHandler* (*create)();
};

/**
 * Descriptor for a machine preset provided by a plugin.
 */
struct MachinePluginInfo {
    const char* machineId;
    MachineDescriptor* (*create)();
};

/**
 * Top-level manifest returned by the plugin entry point.
 */
struct SimPluginManifest {
    int apiVersion;
    const char* pluginId;
    const char* version;

    int coreCount;
    CorePluginInfo* cores;

    int toolchainCount;
    ToolchainPluginInfo* toolchains;

    int deviceCount;
    DevicePluginInfo* devices;

    int machineCount;
    MachinePluginInfo* machines;
};

/**
 * Every plugin .so must export:
 *   extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host);
 */
typedef SimPluginManifest* (*MmemuPluginInitFn)(const SimPluginHostAPI* host);
