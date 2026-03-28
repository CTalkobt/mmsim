#pragma once

#include <cstdint>

/**
 * Interface for devices that produce audio output (e.g. VIC-I, SID, POKEY).
 *
 * Devices accumulate generated samples into an internal ring buffer during
 * tick(). The host audio system pulls samples on demand via pullSamples().
 * Samples are mono, normalised floats in the range [-1.0, 1.0].
 */
class IAudioOutput {
public:
    virtual ~IAudioOutput() {}

    /**
     * The sample rate this device was configured to generate at.
     */
    virtual int nativeSampleRate() const = 0;

    /**
     * Pull up to maxSamples from the device's output ring buffer.
     * Writes mono float samples ([-1.0, 1.0]) to buffer.
     * @return Number of samples actually written (may be less than maxSamples).
     */
    virtual int pullSamples(float* buffer, int maxSamples) = 0;
};
