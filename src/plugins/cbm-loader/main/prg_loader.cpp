#include "prg_loader.h"
#include "libcore/main/machine_desc.h"
#include <fstream>
#include <iostream>
#include <algorithm>

bool PrgLoader::canLoad(const std::string& path) const {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "prg";
}

bool PrgLoader::load(const std::string& path, IBus* bus, MachineDescriptor* machine, uint32_t addr) {
    if (!bus && machine) {
        if (!machine->buses.empty()) {
            bus = machine->buses[0].bus;
        }
    }
    if (!bus) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint8_t header[2];
    if (!file.read((char*)header, 2)) return false;

    // Header is the load address (little endian)
    uint32_t loadAddr = header[0] | (header[1] << 8);
    
    // If addr is provided and not 0, it overrides the header
    if (addr != 0) {
        loadAddr = addr;
    }

    uint8_t byte;
    uint32_t currentAddr = loadAddr;
    while (file.read((char*)&byte, 1)) {
        bus->write8(currentAddr++, byte);
    }

    return true;
}
