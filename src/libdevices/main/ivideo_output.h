#pragma once

#include <cstdint>

/**
 * Interface for devices that produce video output (e.g. VIC-I, VIC-II, VDC).
 */
class IVideoOutput {
public:
    virtual ~IVideoOutput() {}

    /**
     * Information about the current frame geometry.
     */
    struct VideoDimensions {
        int width;
        int height;
        int displayWidth;  // Visible part
        int displayHeight;
    };

    virtual VideoDimensions getDimensions() const = 0;

    /**
     * Render the current frame into an RGBA8888 buffer.
     * @param buffer Pointer to the destination buffer (width * height * 4 bytes).
     */
    virtual void renderFrame(uint32_t* buffer) = 0;
};
