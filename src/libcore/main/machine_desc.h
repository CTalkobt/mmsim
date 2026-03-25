#pragma once

#include "icore.h"
#include "libmem/main/ibus.h"
#include <vector>
#include <string>
#include <functional>

/**
 * Slot for a CPU in a machine.
 */
struct CpuSlot {
    std::string slotName;
    ICore*      cpu;
    IBus*       dataBus;
    IBus*       codeBus;
    IBus*       ioBus;
    bool        active;
    uint64_t    clockDiv;
};

/**
 * Slot for a Bus in a machine.
 */
struct BusSlot {
    std::string busName;
    IBus*       bus;
};

/**
 * Memory overlay definition.
 */
struct MemOverlay {
    std::string path;
    uint32_t    base;
    bool        cpuVisible;
    bool        active;
};

/**
 * Forward declaration of IORegistry.
 */
class IORegistry;
class IKeyboardDevice;

/**
 * Machine Descriptor.
 */
struct MachineDescriptor {
    std::string machineId;
    std::string displayName;
    std::string licenseClass;

    std::vector<CpuSlot> cpus;
    std::vector<BusSlot> buses;

    IORegistry* ioRegistry = nullptr;
    IKeyboardDevice* keyboard = nullptr;

    // Lifecycle hooks
    std::function<void(MachineDescriptor&)> onReset;
    std::function<void(MachineDescriptor&)> onInit;

    // Scheduler
    std::function<int(MachineDescriptor&)> schedulerStep;

    std::vector<MemOverlay> overlays;
};
