#include "pokey.h"
#include "libmem/main/ibus.h"
#include <cstring>
#include <algorithm>

bool POKEY::ioRead(IBus* bus, uint32_t addr, uint8_t* val) {
    uint32_t offset = addr & 0x000F;
    switch (offset) {
        case STIMER: // ALLPOT
            *val = 0xFF; // All pots at max (simplified)
            return true;
        case SKRES:  // KBCODE
            *val = m_kbcode;
            return true;
        case POTGO:  // RANDOM
            *val = (uint8_t)(m_poly17 >> 8);
            return true;
        case SEROUT: // SERIN
            *val = 0xFF;
            return true;
        case IRQEN:  // IRQST
            *val = m_irqst;
            return true;
        case SKCTL:  // SKSTAT
            *val = m_skstat;
            return true;
        default:
            if (offset < 8) {
                *val = m_regs[offset];
                return true;
            }
            *val = 0x00;
            return true;
    }
}

bool POKEY::ioWrite(IBus* bus, uint32_t addr, uint8_t val) {
    uint32_t offset = addr & 0x000F;
    m_regs[offset] = val;

    switch (offset) {
        case AUDCTL:
            // Handled in synthesize/tick
            break;
        case STIMER:
            for (int i = 0; i < 4; ++i) {
                m_channels[i].counter = m_regs[i * 2];
            }
            break;
        case SKRES:
            m_skstat &= ~0x0C; // Clear frame/overrun errors (simplified)
            break;
        case POTGO:
            // Start pot scan (simplified)
            break;
        case IRQEN:
            m_irqen = val;
            updateIrqs();
            break;
        case SKCTL:
            // Serial port control
            break;
    }
    return true;
}

void POKEY::reset() {
    std::memset(m_regs, 0, sizeof(m_regs));
    std::memset(m_channels, 0, sizeof(m_channels));
    std::memset(m_audioBuf, 0, sizeof(m_audioBuf));
    
    m_poly4  = 0x0F;
    m_poly5  = 0x1F;
    m_poly9  = 0x1FF;
    m_poly17 = 0x1FFFF;
    
    m_irqst = 0xFF;
    m_irqen = 0x00;
    m_kbcode = 0xFF;
    m_skstat = 0xFF;
    
    m_audioBufRead  = 0;
    m_audioBufWrite = 0;
    m_audioBufCount = 0;
    m_lastCycles = 0;
    m_sampleFrac = 0;
}

void POKEY::tick(uint64_t cycles) {
    if (cycles <= m_lastCycles) return;
    uint64_t delta = cycles - m_lastCycles;
    m_lastCycles = cycles;

    synthesize(delta);
}

int POKEY::pullSamples(float* buffer, int maxSamples) {
    int count = std::min(maxSamples, m_audioBufCount);
    for (int i = 0; i < count; ++i) {
        buffer[i] = m_audioBuf[m_audioBufRead];
        m_audioBufRead = (m_audioBufRead + 1) % AUDIO_BUF;
    }
    m_audioBufCount -= count;
    return count;
}

void POKEY::updatePoly() {
    // Poly 4: x^4 + x + 1
    uint32_t b4 = ((m_poly4 >> 3) ^ (m_poly4 >> 2)) & 1;
    m_poly4 = ((m_poly4 << 1) | b4) & 0x0F;

    // Poly 5: x^5 + x^3 + 1
    uint32_t b5 = ((m_poly5 >> 4) ^ (m_poly5 >> 2)) & 1;
    m_poly5 = ((m_poly5 << 1) | b5) & 0x1F;

    // Poly 9: x^9 + x^5 + 1
    uint32_t b9 = ((m_poly9 >> 8) ^ (m_poly9 >> 4)) & 1;
    m_poly9 = ((m_poly9 << 1) | b9) & 0x1FF;

    // Poly 17: x^17 + x^5 + 1
    uint32_t b17 = ((m_poly17 >> 16) ^ (m_poly17 >> 4)) & 1;
    m_poly17 = ((m_poly17 << 1) | b17) & 0x1FFFF;
}

void POKEY::updateIrqs() {
    if (m_irqLine) {
        bool asserted = (~m_irqst & m_irqen) != 0;
        m_irqLine->set(asserted);
    }
}

void POKEY::synthesize(uint64_t delta) {
    uint8_t audctl = m_regs[AUDCTL];
    
    for (uint64_t i = 0; i < delta; ++i) {
        // Update LFSRs at 1.79MHz
        updatePoly();

        // Audio clocking logic
        // Ch1 can be 1.79MHz or 64kHz or 15kHz
        // Ch3 can be 1.79MHz or 64kHz or 15kHz
        // Ch2 and Ch4 are 64kHz or 15kHz
        
        bool clk15 = (audctl & CTL_CLK_15);
        // Simplified clocking: just 1.79MHz for now, or scaled
        
        for (int ch = 0; i % 28 == 0 && ch < 4; ++ch) { // Roughly 64kHz if delta is 1.79MHz
             // Real POKEY clocking is more complex. 
             // 1.79MHz / 28 = 63.9kHz
             // 1.79MHz / 114 = 15.7kHz
        }

        // Simplified audio counter update
        for (int ch = 0; ch < 4; ++ch) {
            uint32_t freq = m_regs[ch * 2];
            // Handle joined channels
            if (ch == 0 && (audctl & CTL_CH12_16)) continue;
            if (ch == 2 && (audctl & CTL_CH34_16)) continue;
            
            uint32_t reload = freq + 1;
            if (ch == 1 && (audctl & CTL_CH12_16)) {
                reload = (m_regs[AUDF2] << 8) | m_regs[AUDF1];
                reload += 1;
            }
            if (ch == 3 && (audctl & CTL_CH34_16)) {
                reload = (m_regs[AUDF4] << 8) | m_regs[AUDF3];
                reload += 1;
            }

            // Frequency divider logic
            if (m_channels[ch].counter > 0) {
                m_channels[ch].counter--;
            } else {
                m_channels[ch].counter = reload;
                m_channels[ch].output = !m_channels[ch].output;
                
                // Timer IRQ logic
                if (ch == 0 || ch == 1 || ch == 3) {
                    uint8_t bit = (ch == 0) ? 0x01 : (ch == 1 ? 0x02 : 0x04);
                    if (m_irqen & bit) {
                        m_irqst &= ~bit;
                        updateIrqs();
                    }
                }
            }
        }

        // Sample generation
        m_sampleFrac += m_sampleRate;
        if (m_sampleFrac >= m_clockHz) {
            m_sampleFrac -= m_clockHz;
            
            float mix = 0.0f;
            for (int ch = 0; ch < 4; ++ch) {
                uint8_t audc = m_regs[ch * 2 + 1];
                float vol = (float)(audc & 0x0F) / 15.0f;
                
                bool out = m_channels[ch].output;
                
                // Distortion
                if (!(audc & 0x80)) out &= (m_poly5 & 1);
                if (!(audc & 0x40)) out &= (audctl & CTL_POLY9 ? (m_poly9 & 1) : (m_poly17 & 1));
                if (!(audc & 0x20)) out &= (m_poly4 & 1);
                
                if (audc & 0x10) mix += vol; // Volume only mode
                else mix += (out ? vol : 0.0f);
            }
            
            if (m_audioBufCount < AUDIO_BUF) {
                m_audioBuf[m_audioBufWrite] = (mix / 4.0f);
                m_audioBufWrite = (m_audioBufWrite + 1) % AUDIO_BUF;
                m_audioBufCount++;
            }
        }
    }
}
