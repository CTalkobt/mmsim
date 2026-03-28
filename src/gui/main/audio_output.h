#pragma once

#include "libdevices/main/iaudio_output.h"
#include <atomic>
#include <thread>

/**
 * ALSA streaming audio output.
 *
 * Spawns a background thread that continuously pulls float samples from an
 * IAudioOutput source and writes them to the default ALSA PCM device.
 * Underruns are recovered silently; buffer starvation (source paused) is
 * filled with silence so the ALSA stream stays open.
 */
class AudioOutput {
public:
    AudioOutput() = default;
    ~AudioOutput() { stop(); }

    // Non-copyable
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    /**
     * Open the ALSA device and start the playback thread.
     * @param source  IAudioOutput to pull samples from (must outlive stop()).
     * @return true on success; false if ALSA initialisation failed (no audio).
     */
    bool start(IAudioOutput* source);

    /** Stop playback and join the thread. Safe to call repeatedly. */
    void stop();

    bool isRunning() const { return m_running.load(); }

private:
    void threadFn();

    IAudioOutput*     m_source  = nullptr;
    void*             m_handle  = nullptr; // opaque snd_pcm_t*
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
};
