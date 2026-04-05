#pragma once

#include "libdevices/main/ivideo_output.h"
#include "libdevices/main/io_handler.h"
#include "plugins/devices/crtc6545/main/crtc6545.h"
#include <vector>
#include <cstdint>

/**
 * PET Video Subsystem.
 * Supports PET 2001 (discrete logic) and 4000/8000 (CRTC).
 */
class PetVideo : public IVideoOutput, public IOHandler {
public:
    enum class Model {
        PET_2001,   // 40 columns, discrete logic
        PET_4000,   // 40 columns, CRTC
        PET_8000    // 80 columns, CRTC
    };

    PetVideo(Model model = Model::PET_4000);
    virtual ~PetVideo() = default;

    // IOHandler implementation
    const char* name() const override { return "PetVideo"; }
    uint32_t baseAddr() const override { return 0x8000; }
    uint32_t addrMask() const override { return 0x07FF; } // 2KB Video RAM

    bool ioRead(IBus* bus, uint32_t addr, uint8_t* val) override;
    bool ioWrite(IBus* bus, uint32_t addr, uint8_t val) override;
    void reset() override;
    void tick(uint64_t cycles) override;

    // IVideoOutput implementation
    void renderFrame(uint32_t* buffer) override;
    VideoDimensions getDimensions() const override;

    void setCharRom(const uint8_t* data, uint32_t size) override { m_charRom = data; m_charRomSize = (size_t)size; }
    void setCRTC(CRTC6545* crtc) { m_crtc = crtc; }

private:
    Model m_model;
    uint8_t m_vram[2048];
    const uint8_t* m_charRom = nullptr;
    size_t m_charRomSize = 0;
    CRTC6545* m_crtc = nullptr;

    uint32_t m_pixelOn = 0xFF00FF00;  // Green phosphor
    uint32_t m_pixelOff = 0xFF000000; // Black

    void renderDiscrete(uint32_t* buffer, int width, int height);
    void renderCRTC(uint32_t* buffer, int width, int height);
};
