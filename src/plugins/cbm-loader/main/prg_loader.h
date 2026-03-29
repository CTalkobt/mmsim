#pragma once

#include "libcore/main/image_loader.h"

class PrgLoader : public IImageLoader {
public:
    bool canLoad(const std::string& path) const override;
    bool load(const std::string& path, IBus* bus, MachineDescriptor* machine, uint32_t addr = 0) override;
    const char* name() const override { return "Commodore PRG Loader"; }
};
