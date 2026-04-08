#include "datasette.h"
#include <iostream>

Datasette::Datasette() {
    reset();
}

void Datasette::reset() {
    m_offset = 0;
    m_pulseRemaining = 0;
    m_pulseLevel = true;
    m_playing = false;
    m_motorOn = false;
    m_buttonPressed = false;
}

bool Datasette::mount(const std::string& path) {
    bool ok = m_tape.load(path);
    if (ok) {
        m_offset = 0;
        m_pulseRemaining = 0;
        m_playing = true;
        m_buttonPressed = true;
        if (m_senseLine) m_senseLine->set(false); // Sense low = button pressed
    }
    return ok;
}

void Datasette::play() {
    m_playing = true;
    m_buttonPressed = true;
    if (m_senseLine) m_senseLine->set(false);
}

void Datasette::stop() {
    m_playing = false;
}

void Datasette::rewind() {
    m_offset = 0;
    m_pulseRemaining = 0;
}

void Datasette::tick(uint64_t cycles) {
    if (m_motorLine) {
        // C64 motor control (bit 5) is LOW to turn ON.
        m_motorOn = !m_motorLine->get();
    }

    if (!m_playing || !m_motorOn) return;

    m_pulseRemaining -= (uint32_t)cycles;

    if (m_pulseRemaining <= 0) {
        uint32_t pulse = m_tape.nextPulse(m_offset);
        if (pulse == 0) {
            m_playing = false;
            m_buttonPressed = false;
            if (m_senseLine) m_senseLine->set(true);
            return;
        }
        m_pulseRemaining += (int32_t)pulse;

        // Trigger negative edge on CIA FLAG pin
        if (m_readPulseLine) {
            m_readPulseLine->set(false);
            m_readPulseLine->set(true);
        }
    }
}
