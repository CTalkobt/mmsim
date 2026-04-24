#include "cpu45gs02.h"
#include "libdebug/main/execution_observer.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

static const RegDescriptor s_regDescriptors[] = {
    { "A",  RegWidth::R8,  REGFLAG_ACCUMULATOR, nullptr, nullptr    },
    { "X",  RegWidth::R8,  0,                   nullptr, nullptr    },
    { "Y",  RegWidth::R8,  0,                   nullptr, nullptr    },
    { "Z",  RegWidth::R8,  0,                   nullptr, nullptr    },
    { "B",  RegWidth::R8,  0,                   nullptr, nullptr    },
    { "SP", RegWidth::R16, REGFLAG_SP,          nullptr, nullptr    },
    { "PC", RegWidth::R16, REGFLAG_PC,          nullptr, nullptr    },
    { "P",  RegWidth::R8,  REGFLAG_STATUS,      "F",     "NV-BDIZC" }
};

MOS45GS02::MOS45GS02() : m_bus(nullptr) {
    memset(&m_state, 0, sizeof(m_state));
    reset();
}

void MOS45GS02::reset() {
    m_state.a = 0; m_state.x = 0; m_state.y = 0; m_state.z = 0; m_state.b = 0;
    m_state.sp = 0x01FF; m_state.p = FLAG_U | FLAG_I; m_state.cycles = 0;
    m_state.mapEnabled = false; m_state.haltLine = 0;
    for(int i=0; i<4; i++) m_state.map[i] = i * 0x40; 
    if (m_bus) {
        uint16_t lo = read8(0xFFFC); uint16_t hi = read8(0xFFFD);
        m_state.pc = lo | (hi << 8);
    }
}

int MOS45GS02::regCount() const { return 8; }
const RegDescriptor* MOS45GS02::regDescriptor(int idx) const { return (idx >= 0 && idx < 8) ? &s_regDescriptors[idx] : nullptr; }

uint32_t MOS45GS02::regRead(int idx) const {
    switch (idx) {
        case 0: return m_state.a; case 1: return m_state.x; case 2: return m_state.y; case 3: return m_state.z;
        case 4: return m_state.b; case 5: return m_state.sp; case 6: return m_state.pc; case 7: return m_state.p;
        default: return 0;
    }
}

void MOS45GS02::regWrite(int idx, uint32_t val) {
    switch (idx) {
        case 0: m_state.a = (uint8_t)val; break; case 1: m_state.x = (uint8_t)val; break;
        case 2: m_state.y = (uint8_t)val; break; case 3: m_state.z = (uint8_t)val; break;
        case 4: m_state.b = (uint8_t)val; break; case 5: m_state.sp = (uint16_t)val; break;
        case 6: m_state.pc = (uint16_t)val; break; case 7: m_state.p = (uint8_t)val; break;
    }
}

uint32_t MOS45GS02::translate(uint16_t addr) {
    if (!m_state.mapEnabled) return addr;
    return (m_state.map[addr >> 14] << 8) | (addr & 0x3FFF);
}

uint8_t MOS45GS02::read8(uint16_t addr) { return m_bus ? m_bus->read8(translate(addr)) : 0xFF; }
void MOS45GS02::write8(uint16_t addr, uint8_t val) { if (m_bus) m_bus->write8(translate(addr), val); }
void MOS45GS02::updateNZ(uint8_t val) { m_state.p &= ~(FLAG_N | FLAG_Z); if (!val) m_state.p |= FLAG_Z; if (val & 0x80) m_state.p |= FLAG_N; }
void MOS45GS02::updateNZ16(uint16_t val) { m_state.p &= ~(FLAG_N | FLAG_Z); if (!val) m_state.p |= FLAG_Z; if (val & 0x8000) m_state.p |= FLAG_N; }
void MOS45GS02::push8(uint8_t v) { write8(m_state.sp--, v); }
uint8_t MOS45GS02::pull8() { return read8(++m_state.sp); }
void MOS45GS02::push16(uint16_t v) { push8(v >> 8); push8(v & 0xFF); }
uint16_t MOS45GS02::pull16() { uint8_t l = pull8(); uint8_t h = pull8(); return l | (h << 8); }

int MOS45GS02::step() {
    if (m_state.haltLine) return 1;
    if (m_bus && m_bus->isHaltRequested()) { m_state.haltLine = 1; return 1; }
    uint16_t pc = m_state.pc;
    if (m_observer) {
        DisasmEntry entry; disassembleEntry(m_bus, pc, &entry);
        m_state.pc = pc; if (!m_observer->onStep(this, m_bus, entry)) return 1;
    }

    uint8_t op = read8(m_state.pc++);
    m_state.cycles++;

    auto getAbs = [&]() { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); m_state.cycles+=2; return (uint16_t)((h<<8)|l); };
    auto getZp  = [&]() { uint8_t z=read8(m_state.pc++); m_state.cycles++; return (uint16_t)((m_state.b << 8) | z); };
    auto getAbsX = [&]() { return getAbs() + m_state.x; };
    auto getAbsY = [&]() { return getAbs() + m_state.y; };
    auto getZpX  = [&]() { uint8_t z=read8(m_state.pc++); m_state.cycles++; return (uint16_t)((m_state.b << 8) | (uint8_t)(z + m_state.x)); };
    auto getZpY  = [&]() { uint8_t z=read8(m_state.pc++); m_state.cycles++; return (uint16_t)((m_state.b << 8) | (uint8_t)(z + m_state.y)); };

    switch (op) {
        case 0x00: m_state.haltLine = 1; break; // BRK
        case 0xEA: break; // NOP
        
        // --- 1. Load / Store ---
        case 0xA9: m_state.a = read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break;
        case 0xAD: m_state.a = read8(getAbs()); updateNZ(m_state.a); break;
        case 0xBD: m_state.a = read8(getAbsX()); updateNZ(m_state.a); break;
        case 0xB9: m_state.a = read8(getAbsY()); updateNZ(m_state.a); break;
        case 0xA5: m_state.a = read8(getZp()); updateNZ(m_state.a); break;
        case 0xB5: m_state.a = read8(getZpX()); updateNZ(m_state.a); break;
        case 0xA2: m_state.x = read8(m_state.pc++); updateNZ(m_state.x); m_state.cycles++; break;
        case 0xAE: m_state.x = read8(getAbs()); updateNZ(m_state.x); break;
        case 0xBE: m_state.x = read8(getAbsY()); updateNZ(m_state.x); break;
        case 0xA6: m_state.x = read8(getZp()); updateNZ(m_state.x); break;
        case 0xB6: m_state.x = read8(getZpY()); updateNZ(m_state.x); break;
        case 0xA0: m_state.y = read8(m_state.pc++); updateNZ(m_state.y); m_state.cycles++; break;
        case 0xAC: m_state.y = read8(getAbs()); updateNZ(m_state.y); break;
        case 0xBC: m_state.y = read8(getAbsX()); updateNZ(m_state.y); break;
        case 0xA4: m_state.y = read8(getZp()); updateNZ(m_state.y); break;
        case 0xB4: m_state.y = read8(getZpX()); updateNZ(m_state.y); break;
        case 0xA3: m_state.z = read8(m_state.pc++); updateNZ(m_state.z); m_state.cycles++; break;
        
        case 0x8D: write8(getAbs(), m_state.a); break;
        case 0x9D: write8(getAbsX(), m_state.a); break;
        case 0x99: write8(getAbsY(), m_state.a); break;
        case 0x85: write8(getZp(), m_state.a); break;
        case 0x95: write8(getZpX(), m_state.a); break;
        case 0x8E: write8(getAbs(), m_state.x); break;
        case 0x86: write8(getZp(), m_state.x); break;
        case 0x96: write8(getZpY(), m_state.x); break;
        case 0x8C: write8(getAbs(), m_state.y); break;
        case 0x84: write8(getZp(), m_state.y); break;
        case 0x94: write8(getZpX(), m_state.y); break;
        case 0x9C: write8(getAbs(), m_state.z); break;
        case 0x64: write8(getZp(), 0); break;

        // --- 2. Arithmetic / Logic ---
        case 0x69: { uint8_t v=read8(m_state.pc++); uint16_t s=(uint16_t)m_state.a+v+(m_state.p&FLAG_C); m_state.p&=~(FLAG_V|FLAG_C); if(~(m_state.a^v)&(m_state.a^s)&0x80) m_state.p|=FLAG_V; if(s>0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)s; updateNZ(m_state.a); m_state.cycles++; break; }
        case 0x6D: { uint8_t v=read8(getAbs()); uint16_t s=(uint16_t)m_state.a+v+(m_state.p&FLAG_C); m_state.p&=~(FLAG_V|FLAG_C); if(~(m_state.a^v)&(m_state.a^s)&0x80) m_state.p|=FLAG_V; if(s>0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)s; updateNZ(m_state.a); break; }
        case 0x65: { uint8_t v=read8(getZp()); uint16_t s=(uint16_t)m_state.a+v+(m_state.p&FLAG_C); m_state.p&=~(FLAG_V|FLAG_C); if(~(m_state.a^v)&(m_state.a^s)&0x80) m_state.p|=FLAG_V; if(s>0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)s; updateNZ(m_state.a); break; }
        case 0xE9: { uint8_t v=read8(m_state.pc++); uint16_t d=(uint16_t)m_state.a-v-((m_state.p&FLAG_C)?0:1); m_state.p&=~(FLAG_V|FLAG_C); if((m_state.a^v)&(m_state.a^d)&0x80) m_state.p|=FLAG_V; if(d<=0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)d; updateNZ(m_state.a); m_state.cycles++; break; }
        case 0xED: { uint8_t v=read8(getAbs()); uint16_t d=(uint16_t)m_state.a-v-((m_state.p&FLAG_C)?0:1); m_state.p&=~(FLAG_V|FLAG_C); if((m_state.a^v)&(m_state.a^d)&0x80) m_state.p|=FLAG_V; if(d<=0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)d; updateNZ(m_state.a); break; }
        case 0xE5: { uint8_t v=read8(getZp()); uint16_t d=(uint16_t)m_state.a-v-((m_state.p&FLAG_C)?0:1); m_state.p&=~(FLAG_V|FLAG_C); if((m_state.a^v)&(m_state.a^d)&0x80) m_state.p|=FLAG_V; if(d<=0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)d; updateNZ(m_state.a); break; }
        
        case 0xC9: { uint8_t v=read8(m_state.pc++); uint16_t d=(uint16_t)m_state.a-v; m_state.p &= ~FLAG_C; if(d<=0xFF) m_state.p|=FLAG_C; updateNZ((uint8_t)d); m_state.cycles++; break; }
        case 0xCD: { uint8_t v=read8(getAbs()); uint16_t d=(uint16_t)m_state.a-v; m_state.p &= ~FLAG_C; if(d<=0xFF) m_state.p|=FLAG_C; updateNZ((uint8_t)d); break; }
        case 0xC5: { uint8_t v=read8(getZp()); uint16_t d=(uint16_t)m_state.a-v; m_state.p &= ~FLAG_C; if(d<=0xFF) m_state.p|=FLAG_C; updateNZ((uint8_t)d); break; }

        case 0x29: m_state.a &= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break;
        case 0x09: m_state.a |= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break;
        case 0x49: m_state.a ^= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break;

        // --- 3. Increments / Decrements ---
        case 0xE8: m_state.x++; updateNZ(m_state.x); break;
        case 0xCA: m_state.x--; updateNZ(m_state.x); break;
        case 0xC8: m_state.y++; updateNZ(m_state.y); break;
        case 0x88: m_state.y--; updateNZ(m_state.y); break;
        case 0x1A: m_state.a++; updateNZ(m_state.a); break;
        case 0x3A: m_state.a--; updateNZ(m_state.a); break;
        case 0xEE: { uint16_t a=getAbs(); uint8_t v=read8(a)+1; write8(a,v); updateNZ(v); break; }
        case 0xE6: { uint16_t a=getZp(); uint8_t v=read8(a)+1; write8(a,v); updateNZ(v); break; }
        case 0xCE: { uint16_t a=getAbs(); uint8_t v=read8(a)-1; write8(a,v); updateNZ(v); break; }
        case 0xC6: { uint16_t a=getZp(); uint8_t v=read8(a)-1; write8(a,v); updateNZ(v); break; }

        // --- 4. Transfers ---
        case 0xAA: m_state.x = m_state.a; updateNZ(m_state.x); break;
        case 0x8A: m_state.a = m_state.x; updateNZ(m_state.a); break;
        case 0xA8: m_state.y = m_state.a; updateNZ(m_state.y); break;
        case 0x98: m_state.a = m_state.y; updateNZ(m_state.a); break;
        case 0x4B: m_state.z = m_state.a; updateNZ(m_state.z); break;
        case 0x6B: m_state.a = m_state.z; updateNZ(m_state.a); break;
        case 0x5B: m_state.b = m_state.a; updateNZ(m_state.b); break;
        case 0x7B: m_state.a = m_state.b; updateNZ(m_state.a); break;
        case 0xBA: m_state.x = (uint8_t)(m_state.sp & 0xFF); updateNZ(m_state.x); break;
        case 0x9A: m_state.sp = (m_state.sp & 0xFF00) | m_state.x; break;

        // --- 5. Flags ---
        case 0x18: m_state.p &= ~FLAG_C; break;
        case 0x38: m_state.p |=  FLAG_C; break;
        case 0x58: m_state.p &= ~FLAG_I; break;
        case 0x78: m_state.p |=  FLAG_I; break;
        case 0xD8: m_state.p &= ~FLAG_D; break;
        case 0xF8: m_state.p |=  FLAG_D; break;
        case 0xB8: m_state.p &= ~FLAG_V; break;

        // --- 6. Stack ---
        case 0x48: push8(m_state.a); break;
        case 0x68: m_state.a = pull8(); updateNZ(m_state.a); break;
        case 0x08: push8(m_state.p); break;
        case 0x28: m_state.p = pull8() | FLAG_U; break;
        case 0xDA: push8(m_state.x); break;
        case 0xFA: m_state.x = pull8(); updateNZ(m_state.x); break;
        case 0x5A: push8(m_state.y); break;
        case 0x7A: m_state.y = pull8(); updateNZ(m_state.y); break;
        case 0xDB: push8(m_state.z); break;
        case 0xFB: m_state.z = pull8(); updateNZ(m_state.z); break;

        case 0xF4: { uint16_t v = getAbs(); push16(v); break; } // PHW (abs)
        case 0xE2: { uint16_t l = read8(m_state.pc++); uint16_t h = read8(m_state.pc++); uint16_t v = l | (h << 8); m_state.cycles += 2; push16(v); break; } // PHW #imm

        // --- 7. Flow Control ---
        case 0x4C: m_state.pc = getAbs(); break;
        case 0x6C: { uint16_t a=getAbs(); m_state.pc = read8(a)|(read8(a+1)<<8); break; }
        case 0x20: { uint16_t t=getAbs(); uint16_t r=m_state.pc-1; push16(r); m_state.pc=t; break; } // JSR
        case 0x60: { m_state.pc = pull16() + 1; break; } // RTS
        case 0x40: { m_state.p = pull8() | FLAG_U; m_state.pc = pull16(); break; } // RTI
        case 0x80: { int8_t r=(int8_t)read8(m_state.pc++); m_state.pc+=r; break; } // BRA
        case 0x62: { int8_t r=(int8_t)read8(m_state.pc++); uint16_t ret=m_state.pc-1; push16(ret); m_state.pc+=r; break; } // BSR
        case 0x63: { m_state.pc = pull16() + 1; break; } // RTN (same as RTS here, but 65CE02 specifies RTN)

        case 0xF0: { int8_t r=(int8_t)read8(m_state.pc++); if(m_state.p&FLAG_Z) m_state.pc+=r; break; }
        case 0xD0: { int8_t r=(int8_t)read8(m_state.pc++); if(!(m_state.p&FLAG_Z)) m_state.pc+=r; break; }
        case 0xB0: { int8_t r=(int8_t)read8(m_state.pc++); if(m_state.p&FLAG_C) m_state.pc+=r; break; }
        case 0x90: { int8_t r=(int8_t)read8(m_state.pc++); if(!(m_state.p&FLAG_C)) m_state.pc+=r; break; }
        case 0x10: { int8_t r=(int8_t)read8(m_state.pc++); if(!(m_state.p&FLAG_N)) m_state.pc+=r; break; }
        case 0x30: { int8_t r=(int8_t)read8(m_state.pc++); if(m_state.p&FLAG_N) m_state.pc+=r; break; }
        case 0x50: { int8_t r=(int8_t)read8(m_state.pc++); if(!(m_state.p&FLAG_V)) m_state.pc+=r; break; }
        case 0x70: { int8_t r=(int8_t)read8(m_state.pc++); if(m_state.p&FLAG_V) m_state.pc+=r; break; }

        // --- 8. Bit Manipulation ---
        case 0x24: { uint8_t v=read8(getZp()); m_state.p &= ~(FLAG_Z|FLAG_N|FLAG_V); if(!(m_state.a&v)) m_state.p|=FLAG_Z; if(v&0x80) m_state.p|=FLAG_N; if(v&0x40) m_state.p|=FLAG_V; break; }
        case 0x2C: { uint8_t v=read8(getAbs()); m_state.p &= ~(FLAG_Z|FLAG_N|FLAG_V); if(!(m_state.a&v)) m_state.p|=FLAG_Z; if(v&0x80) m_state.p|=FLAG_N; if(v&0x40) m_state.p|=FLAG_V; break; }
        case 0x89: { uint8_t v=read8(m_state.pc++); m_state.cycles++; updateNZ(m_state.a&v); break; }
        
        // RMBx / SMBx (Rockwell/W65C02/45GS02)
        case 0x07: case 0x17: case 0x27: case 0x37: case 0x47: case 0x57: case 0x67: case 0x77: {
            uint16_t a = getZp(); uint8_t v = read8(a); uint8_t bit = (op >> 4); 
            v &= ~(1 << bit); write8(a, v); break;
        }
        case 0x87: case 0x97: case 0xA7: case 0xB7: case 0xC7: case 0xD7: case 0xE7: case 0xF7: {
            uint16_t a = getZp(); uint8_t v = read8(a); uint8_t bit = ((op - 0x87) >> 4); 
            v |= (1 << bit); write8(a, v); break;
        }
        // BBRx / BBSx
        case 0x0F: case 0x1F: case 0x2F: case 0x3F: case 0x4F: case 0x5F: case 0x6F: case 0x7F: {
            uint16_t a = getZp(); uint8_t v = read8(a); int8_t r = (int8_t)read8(m_state.pc++); 
            uint8_t bit = (op >> 4);
            if (!(v & (1 << bit))) m_state.pc += r; 
            break;
        }
        case 0x8F: case 0x9F: case 0xAF: case 0xBF: case 0xCF: case 0xDF: case 0xEF: case 0xFF: {
            uint16_t a = getZp(); uint8_t v = read8(a); int8_t r = (int8_t)read8(m_state.pc++); 
            uint8_t bit = ((op - 0x8F) >> 4);
            if (v & (1 << bit)) m_state.pc += r; 
            break;
        }

        // --- 9. Shifts ---
        case 0x0A: { uint8_t c=m_state.a>>7; m_state.a<<=1; m_state.p=(m_state.p&~FLAG_C)|c; updateNZ(m_state.a); break; }
        case 0x4A: { uint8_t c=m_state.a&1; m_state.a>>=1; m_state.p=(m_state.p&~FLAG_C)|c; updateNZ(m_state.a); break; }
        case 0x2A: { uint8_t c=(m_state.p&FLAG_C)?1:0; m_state.p=(m_state.p&~FLAG_C)|(m_state.a>>7); m_state.a=(m_state.a<<1)|c; updateNZ(m_state.a); break; }
        case 0x6A: { uint8_t c=(m_state.p&FLAG_C)?0x80:0; m_state.p=(m_state.p&~FLAG_C)|(m_state.a&1); m_state.a=(m_state.a>>1)|c; updateNZ(m_state.a); break; }

        // --- 10. 45GS02 Extensions ---
        case 0x42: { m_state.a = -(int8_t)m_state.a; updateNZ(m_state.a); m_state.cycles++; break; } // NEG
        case 0x5C: { m_state.mapEnabled = true; break; } // MAP
        case 0x7C: { m_state.mapEnabled = false; break; } // EOM

        // 16-bit word operations
        case 0xE3: { uint16_t a = getZp(); uint16_t v = (uint16_t)read8(a) | ((uint16_t)read8(a+1) << 8); v++; write8(a, (uint8_t)(v&0xFF)); write8(a+1, (uint8_t)(v>>8)); updateNZ16(v); break; } // INW
        case 0xC3: { uint16_t a = getZp(); uint16_t v = (uint16_t)read8(a) | ((uint16_t)read8(a+1) << 8); v--; write8(a, (uint8_t)(v&0xFF)); write8(a+1, (uint8_t)(v>>8)); updateNZ16(v); break; } // DEW
        case 0x53: { uint16_t a = getZp(); uint16_t v = (uint16_t)read8(a) | ((uint16_t)read8(a+1) << 8); m_state.p = (m_state.p & ~FLAG_C) | (v >> 15); v <<= 1; write8(a, (uint8_t)(v&0xFF)); write8(a+1, (uint8_t)(v>>8)); updateNZ16(v); break; } // ASW
        case 0xB3: { uint16_t a = getZp(); uint16_t v = (uint16_t)read8(a) | ((uint16_t)read8(a+1) << 8); uint16_t c = (m_state.p & FLAG_C) ? 1 : 0; m_state.p = (m_state.p & ~FLAG_C) | (v >> 15); v = (v << 1) | c; write8(a, (uint8_t)(v&0xFF)); write8(a+1, (uint8_t)(v>>8)); updateNZ16(v); break; } // ROW

        default: m_state.haltLine = 1; break;
    }
    return 1;
}

void MOS45GS02::triggerIrq() {}
void MOS45GS02::triggerNmi() {}

int MOS45GS02::disassembleOne(IBus* bus, uint32_t addr, char* buf, int bufsz) {
    uint8_t op = bus->peek8(addr);
    switch (op) {
        case 0x00: snprintf(buf, bufsz, "BRK"); return 1;
        case 0xEA: snprintf(buf, bufsz, "NOP"); return 1;
        case 0xA9: snprintf(buf, bufsz, "LDA #$%02X", bus->peek8(addr+1)); return 2;
        case 0x8D: snprintf(buf, bufsz, "STA $%04X", bus->peek8(addr+1)|(bus->peek8(addr+2)<<8)); return 3;
        default: snprintf(buf, bufsz, "??? ($%02X)", op); return 1;
    }
}

int MOS45GS02::disassembleEntry(IBus* bus, uint32_t addr, void* entryOut) {
    DisasmEntry* e = (DisasmEntry*)entryOut; e->addr = addr;
    char buf[64]; e->bytes = disassembleOne(bus, addr, buf, sizeof(buf)); e->complete = buf;
    uint8_t op = bus->peek8(addr); e->isCall = (op==0x20); e->isReturn = (op==0x60||op==0x40);
    e->isBranch = (op&0x1F)==0x10 || (op==0x80); return e->bytes;
}

void MOS45GS02::saveState(uint8_t* buf) const { memcpy(buf, &m_state, sizeof(m_state)); }
void MOS45GS02::loadState(const uint8_t* buf) { memcpy(&m_state, buf, sizeof(m_state)); }
int MOS45GS02::isCallAt(IBus* bus, uint32_t addr) { return bus->peek8(addr) == 0x20 ? 3 : 0; }
bool MOS45GS02::isReturnAt(IBus* bus, uint32_t addr) { uint8_t op = bus->peek8(addr); return op == 0x60 || op == 0x40; }
bool MOS45GS02::isProgramEnd(IBus* bus) { 
    if (m_state.haltLine) return true;
    if (bus && bus->isHaltRequested()) return true;
    return false;
}
int MOS45GS02::writeCallHarness(IBus* bus, uint32_t scratch, uint32_t routineAddr) {
    bus->write8(scratch, 0x20); bus->write8(scratch+1, routineAddr&0xFF); bus->write8(scratch+2, routineAddr>>8); bus->write8(scratch+3, 0x00); return 4;
}
