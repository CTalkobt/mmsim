#pragma once

#include "libdebug/main/execution_observer.h"
#include <string>
#include <map>
#include <vector>

class KernalHLE : public ExecutionObserver {
public:
    KernalHLE();
    virtual ~KernalHLE() {}

    bool onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) override;
    void onMachineLoad(MachineDescriptor* desc) override { m_currentMachine = desc; }

    void setHostPath(const std::string& path) { m_hostPath = path; }
    const std::string& getHostPath() const { return m_hostPath; }

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

private:
    struct Vectors {
        uint32_t load;
        uint32_t save;
    };

    bool m_enabled;
    std::string m_hostPath;
    std::map<std::string, Vectors> m_machineVectors;
    MachineDescriptor* m_currentMachine = nullptr;

    void handleLoad(ICore* cpu, IBus* bus);
    void handleSave(ICore* cpu, IBus* bus);

    // Helpers to read KERNAL variables from RAM
    uint8_t getDevice(ICore* cpu, IBus* bus);
    uint8_t getSecondaryAddress(ICore* cpu, IBus* bus);
    std::string getFilename(ICore* cpu, IBus* bus);
    
    void setStatus(IBus* bus, uint8_t status);
    void setCarry(ICore* cpu, bool set);
};
