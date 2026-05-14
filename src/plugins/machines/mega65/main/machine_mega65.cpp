#include "machine_mega65.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/core_registry.h"
#include "libdevices/main/io_registry.h"
#include "libmem/main/sparse_memory_bus.h"
#include "plugins/devices/map_mmu/main/map_mmu.h"
#include "plugins/devices/map_mmu/main/c64_bank_controller.h"
#include "plugins/devices/map_mmu/main/key_register.h"
#include "util/path_util.h"
#include <cstring>

MachineDescriptor* Mega65MachineFactory::create() {
    auto* desc = new MachineDescriptor();

    desc->machineId    = "mega65";
    desc->displayName  = "MEGA65";
    desc->licenseClass = "proprietary";

    // -----------------------------------------------------------------------
    // Create 28-bit SparseMemoryBus (physical address space)
    // -----------------------------------------------------------------------
    auto* physBus = new SparseMemoryBus("phys_bus", 28);
    desc->buses.push_back({"phys_bus", physBus});

    // -----------------------------------------------------------------------
    // Create MapMmu (virtual address translator)
    // -----------------------------------------------------------------------
    auto* mmu = new MapMmu("mmu", physBus);
    desc->buses.push_back({"mmu", mmu});

    // -----------------------------------------------------------------------
    // Create C64BankController (ROM overlay banking for C64 compatibility)
    // -----------------------------------------------------------------------
    auto* bankCtrl = new C64BankController(physBus);
    bankCtrl->setMapMmu(mmu);

    // -----------------------------------------------------------------------
    // Create KEY register ($D02F) for I/O personality switching
    // -----------------------------------------------------------------------
    auto* keyReg = new KeyRegister();

    // -----------------------------------------------------------------------
    // Create IORegistry and register handlers
    // -----------------------------------------------------------------------
    auto* io = new IORegistry();
    io->registerHandler(bankCtrl);
    io->registerHandler(keyReg);
    desc->ioRegistry = io;
    desc->deleters.push_back([bankCtrl]() { delete bankCtrl; });
    desc->deleters.push_back([keyReg]() { delete keyReg; });

    // -----------------------------------------------------------------------
    // Create 45GS02 CPU
    // -----------------------------------------------------------------------
    CoreRegistry& reg = CoreRegistry::instance();
    ICore* cpu = reg.createCore("45GS02");
    if (!cpu) {
        delete desc;
        return nullptr;
    }

    cpu->setDataBus(mmu);
    cpu->setCodeBus(mmu);

    // Wire MapMmu to CPU so MAP instruction can update mapping state
    cpu->setMapMmu(static_cast<IMapController*>(mmu));

    desc->cpus.push_back({"main", cpu, mmu, mmu, nullptr, true, 1});

    return desc;
}
