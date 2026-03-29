#pragma once

#include "libcore/main/image_loader.h"
#include <string>
#include <vector>
#include <cstdint>

class CbmCartridgeHandler : public ICartridgeHandler {
public:
    CbmCartridgeHandler(const std::string& path);
    ~CbmCartridgeHandler() override;

    bool attach(IBus* bus, MachineDescriptor* machine) override;
    void eject(IBus* bus) override;
    CartridgeMetadata metadata() const override;

private:
    std::string m_path;
    std::string m_displayName;
    uint16_t m_cartType;
    uint8_t m_exrom;
    uint8_t m_game;

    struct Bank {
        uint16_t bankNum;
        uint32_t loadAddr;
        std::vector<uint8_t> data;
    };
    std::vector<Bank> m_banks;

    bool loadCrt();
};
