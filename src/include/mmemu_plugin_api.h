#pragma once

#include <cstdint>

// mmemu public plugin ABI version — upper 16 bits: major, lower 16 bits: minor.
// Major version changes are breaking; minor version changes are additive.
#define MMEMU_PLUGIN_API_VERSION  0x00010000u
#define MMEMU_PLUGIN_API_MAJOR(v) ((v) >> 16)
#define MMEMU_PLUGIN_API_MINOR(v) ((v) & 0xFFFFu)

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
    class CoreRegistry*      coreRegistry;
    class MachineRegistry*   machineRegistry;
    class DeviceRegistry*    deviceRegistry;
    class ToolchainRegistry* toolchainRegistry;
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
    uint32_t apiVersion;
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
