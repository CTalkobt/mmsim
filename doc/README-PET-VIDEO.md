# mmemu-plugin-pet-video: PET Video Subsystem Implementation

The PET Video Subsystem simulates the video hardware found in various Commodore PET models, supporting both the early discrete logic models (PET 2001) and the later CRTC-based models (4000 and 8000 series). It is implemented as an `IOHandler` and `IVideoOutput` plugin within `mmemu-plugin-pet-video.so`.

---

## 1. Intent

The PET Video Subsystem is responsible for rendering the character-based display of the Commodore PET. It manages Video RAM (VRAM) and provides an RGBA frame buffer for the host GUI.

---

## 2. Functionality

### 2.1 Video Modes

The subsystem supports three primary models:
- **PET_2001**: 40-column display using early discrete logic timing.
- **PET_4000**: 40-column display using the MOS 6545 CRTC.
- **PET_8000**: 80-column display using the MOS 6545 CRTC.

### 2.2 Character Generation

Rendering is performed by fetching character codes from VRAM and looking up the corresponding 8x8 (or larger) glyph data in a character ROM.
- **Inversion**: PET character codes use bit 7 to indicate inverse video.
- **Color**: PET displays are typically monochromatic (Green or Amber phosphor). The simulator uses a hardcoded green-phosphor palette (`0xFF00FF00` for "on" pixels).

### 2.3 Integration with CRTC

For CRTC-based models, the `PetVideo` class interfaces with a `CRTC6545` instance to determine screen geometry and memory addressing. In the discrete logic model (PET 2001), fixed timing and a standard 40x25 layout are used.

### 2.4 Rendering (IVideoOutput)

`renderFrame(buffer)` produces a monochromatic RGBA8888 image.
- **Resolution**: 320x200 for 40-column models, 640x200 for 80-column models.

---

## 3. Register Map (Host View)

Video RAM is mapped at `$8000` in Commodore PET systems:

| Address Range | Size | Description |
|---------------|------|-------------|
| $8000–$87FF | 2 KB | Video RAM (Character codes) |

---

## 4. Dependencies

- **Depends on**: `IVideoOutput` (from `libdevices`), `CRTC6545` (from `crtc6545` plugin)
- **Required by**: PET Machine Plugin (Phase 12.5)

---

## 5. Implementation Details

- **Source Location**: `src/plugins/devices/pet_video/main/pet_video.h/cpp`
- **Plugin**: `lib/mmemu-plugin-pet-video.so`
- **Class**: `PetVideo : public IVideoOutput, public IOHandler`
- **Registration Names**: `"pet_video_2001"`, `"pet_video_4000"`, `"pet_video_8000"`
- **Base Address**: `$8000`
- **Address Mask**: `0x07FF` (2 KB window)
