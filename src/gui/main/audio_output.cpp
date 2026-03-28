#include "audio_output.h"
#include <alsa/asoundlib.h>
#include <cstring>
#include <iostream>

// Period size in frames (samples). 1024 frames @ 44100 Hz ≈ 23 ms latency.
static constexpr snd_pcm_uframes_t PERIOD_FRAMES = 1024;
static constexpr unsigned int      SAMPLE_RATE   = 44100;

bool AudioOutput::start(IAudioOutput* source) {
    stop(); // ensure any previous stream is closed

    m_source = source;

    snd_pcm_t* pcm = nullptr;
    int err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        std::cerr << "AudioOutput: cannot open PCM device: "
                  << snd_strerror(err) << std::endl;
        return false;
    }

    // Configure: mono float32, 44100 Hz, ~50 ms target latency.
    err = snd_pcm_set_params(pcm,
        SND_PCM_FORMAT_FLOAT,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        1,           // channels: mono
        SAMPLE_RATE,
        1,           // allow software resampling
        50000);      // latency in µs
    if (err < 0) {
        std::cerr << "AudioOutput: set_params failed: "
                  << snd_strerror(err) << std::endl;
        snd_pcm_close(pcm);
        return false;
    }

    m_handle  = pcm;
    m_running = true;
    m_thread  = std::thread(&AudioOutput::threadFn, this);
    return true;
}

void AudioOutput::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
    if (m_handle) {
        snd_pcm_drain(static_cast<snd_pcm_t*>(m_handle));
        snd_pcm_close(static_cast<snd_pcm_t*>(m_handle));
        m_handle = nullptr;
    }
    m_source = nullptr;
}

void AudioOutput::threadFn() {
    auto* pcm = static_cast<snd_pcm_t*>(m_handle);
    float buf[PERIOD_FRAMES];

    while (m_running) {
        // Pull samples from the device's ring buffer.
        int got = m_source ? m_source->pullSamples(buf, (int)PERIOD_FRAMES) : 0;

        // Pad with silence if the source hasn't generated enough yet
        // (machine paused, first frame, etc.).
        if (got < (int)PERIOD_FRAMES)
            std::memset(buf + got, 0, ((int)PERIOD_FRAMES - got) * sizeof(float));

        snd_pcm_sframes_t written = snd_pcm_writei(pcm, buf, PERIOD_FRAMES);
        if (written < 0) {
            // Recover from underrun (EPIPE) or suspend (ESTRPIPE).
            written = snd_pcm_recover(pcm, (int)written, /*silent=*/1);
            if (written < 0) {
                std::cerr << "AudioOutput: unrecoverable error: "
                          << snd_strerror((int)written) << std::endl;
                break;
            }
        }
    }
}
