#include "test_harness.h"
#include "plugins/devices/crtc6545/main/crtc6545.h"
#include "plugins/devices/pet_video/main/pet_video.h"
#include <cstring>

TEST_CASE(CRTC6545_Registers) {
    CRTC6545 crtc;
    
    // Write to R0 (Horizontal Total)
    crtc.ioWrite(nullptr, 0xE880, 0);
    crtc.ioWrite(nullptr, 0xE881, 63);
    ASSERT(crtc.getReg(0) == 63);

    // Write to R12/R13 (Start Address)
    crtc.ioWrite(nullptr, 0xE880, 12);
    crtc.ioWrite(nullptr, 0xE881, 0x12);
    crtc.ioWrite(nullptr, 0xE880, 13);
    crtc.ioWrite(nullptr, 0xE881, 0x34);
    ASSERT(crtc.getReg(12) == 0x12);
    ASSERT(crtc.getReg(13) == 0x34);
}

TEST_CASE(PetVideo_Geometry) {
    PetVideo video2001(PetVideo::Model::PET_2001);
    auto dims = video2001.getDimensions();
    ASSERT(dims.width == 320);
    ASSERT(dims.height == 200);

    PetVideo video8000(PetVideo::Model::PET_8000);
    dims = video8000.getDimensions();
    ASSERT(dims.width == 640);
    ASSERT(dims.height == 200);
}

TEST_CASE(PetVideo_Render_Discrete) {
    PetVideo video(PetVideo::Model::PET_2001);
    uint8_t charRom[2048];
    std::memset(charRom, 0xAA, sizeof(charRom)); // Alternate dots
    video.setCharRom(charRom, sizeof(charRom));

    // Write 'A' to first char of VRAM
    video.ioWrite(nullptr, 0x8000, 0x01); 

    uint32_t buffer[320 * 200];
    video.renderFrame(buffer);

    // Verify first pixel of 'A' (character 1)
    // Char 1, line 0 is at offset 1*8 = 8 in ROM
    // bit 7 of 0xAA is 1 -> pixelOn (Green)
    ASSERT(buffer[0] == 0xFF00FF00);
}

TEST_CASE(PetVideo_Inverse) {
    PetVideo video(PetVideo::Model::PET_2001);
    uint8_t charRom[2048];
    std::memset(charRom, 0xFF, sizeof(charRom)); // All dots
    video.setCharRom(charRom, sizeof(charRom));

    // Write inverse space ($A0) to VRAM
    video.ioWrite(nullptr, 0x8000, 0xA0); 

    uint32_t buffer[320 * 200];
    video.renderFrame(buffer);

    // Char $20 (32), line 0 is at offset 32*8 = 256 in ROM
    // Inverse bit 7 is set -> bits 0xFF become 0x00 -> pixelOff (Black)
    ASSERT(buffer[0] == 0xFF000000);
}
