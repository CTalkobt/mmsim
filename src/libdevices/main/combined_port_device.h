#pragma once

#include "iport_device.h"
#include <vector>

/**
 * A port device that combines multiple other port devices.
 * For active-low logic (standard for Commodore), it performs a bitwise AND
 * of all inputs.
 */
class CombinedPortDevice : public IPortDevice {
public:
    CombinedPortDevice() = default;
    ~CombinedPortDevice() override = default;

    void addDevice(IPortDevice* device) {
        if (device) m_devices.push_back(device);
    }

    uint8_t readPort() override {
        uint8_t result = 0xFF;
        for (auto* dev : m_devices) {
            result &= dev->readPort();
        }
        return result;
    }

    void writePort(uint8_t val) override {
        for (auto* dev : m_devices) {
            dev->writePort(val);
        }
    }

    void setDdr(uint8_t ddr) override {
        for (auto* dev : m_devices) {
            dev->setDdr(ddr);
        }
    }

private:
    std::vector<IPortDevice*> m_devices;
};
