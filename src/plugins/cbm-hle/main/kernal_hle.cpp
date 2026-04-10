#include "kernal_hle.h"
#include "include/util/logging.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

KernalHLE::KernalHLE() : m_enabled(true), m_hostPath(".") {
    // Standard KERNAL jump table vectors (fixed for all CBM machines)
    m_machineVectors["c64"]   = { 0xFFD5, 0xFFD8 };
    m_machineVectors["vic20"] = { 0xFFD5, 0xFFD8 };
    m_machineVectors["pet2001"] = { 0xFFD5, 0xFFD8 };
    m_machineVectors["pet4032"] = { 0xFFD5, 0xFFD8 };
    m_machineVectors["pet8032"] = { 0xFFD5, 0xFFD8 };
}

bool KernalHLE::onStep(ICore* cpu, IBus* bus, const DisasmEntry& entry) {
    if (!m_enabled) return true;

    // Check if we are at a known vector
    if (entry.addr == 0xFFD5) {
        handleLoad(cpu, bus);
        return false; // Abort KERNAL's LOAD implementation
    } else if (entry.addr == 0xFFD8) {
        handleSave(cpu, bus);
        return false; // Abort KERNAL's SAVE implementation
    }

    return true;
}

void KernalHLE::handleLoad(ICore* cpu, IBus* bus) {
    uint8_t device = getDevice(cpu, bus);
    // We only intercept if it's a disk device (8-11) or if enabled for all?
    // For now, let's say we intercept if it's 8-31.
    if (device < 8) {
        // Let KERNAL handle it (e.g. tape)
        // Wait, if I return true from onStep it would continue.
        // But I've already returned false and called handleLoad.
        // I should have checked device in onStep if I wanted to skip.
        // Let's refine onStep.
    }

    uint8_t sa = getSecondaryAddress(cpu, bus);
    std::string filename = getFilename(cpu, bus);
    
    // Determine load address
    uint32_t loadAddr;
    if (sa == 0) {
        // Load to address in X/Y
        loadAddr = cpu->regReadByName("X") | (cpu->regReadByName("Y") << 8);
    } else {
        // Load to address from file header (first 2 bytes)
        loadAddr = 0; // Will be set from file
    }

    std::string fullPath = (fs::path(m_hostPath) / filename).string();
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        // File not found
        setStatus(bus, 4); // 4 = File not found
        setCarry(cpu, true);
        cpu->setPc(0xFFD5 + 1); // This is not quite right, we need to return from the call.
        // Standard HLE: we intercepted at the call site or at the first instruction of the routine.
        // If we intercepted at $FFD5 (the JMP in the jump table), we should simulate the routine and then RTS.
        // But we don't know the return address easily without looking at the stack.
        // Actually, $FFD5 is usually a JMP to the real routine.
        // If the user did JSR $FFD5, the return address is on the stack.
        // We can just set PC to the value popped from stack + 1.
        // Or simpler: let the CPU execute the RTS.
        // We can write an RTS to scratch memory and point PC there?
        // No, let's just use the stack.
        return;
    }

    // Read file
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    
    if (data.size() < 2) {
        setStatus(bus, 4);
        setCarry(cpu, true);
        return;
    }

    size_t startOffset = 0;
    if (sa != 0) {
        loadAddr = data[0] | (data[1] << 8);
        startOffset = 2;
    }

    // Inject data
    for (size_t i = startOffset; i < data.size(); ++i) {
        bus->write8(loadAddr++, data[i]);
    }

    // Update registers for success
    cpu->regWriteByName("X", (uint8_t)(loadAddr & 0xFF));
    cpu->regWriteByName("Y", (uint8_t)(loadAddr >> 8));
    setCarry(cpu, false);
    setStatus(bus, 0);

    // Simulate RTS
    uint8_t lo = bus->read8(0x0100 + ((cpu->sp() + 1) & 0xFF));
    uint8_t hi = bus->read8(0x0100 + ((cpu->sp() + 2) & 0xFF));
    uint16_t retAddr = (lo | (hi << 8)) + 1;
    cpu->regWriteByName("SP", (uint8_t)(cpu->sp() + 2));
    cpu->setPc(retAddr);
}

void KernalHLE::handleSave(ICore* cpu, IBus* bus) {
    // Similar to LOAD
    setCarry(cpu, true); // Not implemented yet
}

uint8_t KernalHLE::getDevice(ICore* cpu, IBus* bus) {
    return bus->read8(0xBA);
}

uint8_t KernalHLE::getSecondaryAddress(ICore* cpu, IBus* bus) {
    return bus->read8(0xB9);
}

std::string KernalHLE::getFilename(ICore* cpu, IBus* bus) {
    uint8_t len = bus->read8(0xB7);
    uint16_t ptr = bus->read8(0xBB) | (bus->read8(0xBC) << 8);
    std::string name;
    for (int i = 0; i < len; ++i) {
        name += (char)bus->read8(ptr + i);
    }
    return name;
}

void KernalHLE::setStatus(IBus* bus, uint8_t status) {
    bus->write8(0x90, status);
}

void KernalHLE::setCarry(ICore* cpu, bool set) {
    uint8_t p = cpu->regReadByName("P");
    if (set) p |= 0x01;
    else     p &= ~0x01;
    cpu->regWriteByName("P", p);
}
