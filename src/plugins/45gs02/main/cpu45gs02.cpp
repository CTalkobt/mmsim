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
    m_state.a = 0;
    m_state.x = 0;
    m_state.y = 0;
    m_state.z = 0;
    m_state.b = 0;
    m_state.sp = 0x01FF; 
    m_state.p = FLAG_U | FLAG_I;
    m_state.cycles = 0;
    m_state.mapEnabled = false;
    m_state.haltLine = 0;

    if (m_bus) {
        uint8_t lo = read8(0xFFFC);
        uint8_t hi = read8(0xFFFD);
        m_state.pc = (uint16_t)lo | ((uint16_t)hi << 8);
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
    int slot = addr >> 14;
    return (m_state.map[slot] << 8) | (addr & 0x3FFF);
}

uint8_t MOS45GS02::read8(uint16_t addr) { return m_bus ? m_bus->read8(translate(addr)) : 0xFF; }
void MOS45GS02::write8(uint16_t addr, uint8_t val) { if (m_bus) m_bus->write8(translate(addr), val); }

void MOS45GS02::updateNZ(uint8_t val) {
    m_state.p &= ~(FLAG_N | FLAG_Z);
    if (val == 0) m_state.p |= FLAG_Z;
    if (val & 0x80) m_state.p |= FLAG_N;
}

int MOS45GS02::step() {
    if (m_state.haltLine) return 1;
    if (m_bus && m_bus->isHaltRequested()) {
        m_state.haltLine = 1;
        return 1;
    }
    uint16_t pc = m_state.pc;
    if (m_observer) {
        DisasmEntry entry; disassembleEntry(m_bus, pc, &entry);
        m_state.pc = pc; if (!m_observer->onStep(this, m_bus, entry)) return 1;
    }

    uint8_t opcode = read8(m_state.pc++);
    m_state.cycles++;

    switch (opcode) {
        case 0x00: m_state.haltLine = 1; break; // BRK
        case 0xEA: break; // NOP
        
        // LDA/LDX/LDY/LDZ Immediate
        case 0xA9: m_state.a = read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break;
        case 0xA2: m_state.x = read8(m_state.pc++); updateNZ(m_state.x); m_state.cycles++; break;
        case 0xA0: m_state.y = read8(m_state.pc++); updateNZ(m_state.y); m_state.cycles++; break;
        case 0xA3: m_state.z = read8(m_state.pc++); updateNZ(m_state.z); m_state.cycles++; break;

        // STA/STX/STY/STZ Absolute
        case 0x8D: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); write8((h<<8)|l, m_state.a); m_state.cycles+=2; break; }
        case 0x8E: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); write8((h<<8)|l, m_state.x); m_state.cycles+=2; break; }
        case 0x8C: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); write8((h<<8)|l, m_state.y); m_state.cycles+=2; break; }
        case 0x9C: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); write8((h<<8)|l, m_state.z); m_state.cycles+=2; break; }

        // Arithmetic Immediate
        case 0x69: { uint8_t v=read8(m_state.pc++); uint16_t s=(uint16_t)m_state.a+v+(m_state.p&FLAG_C); m_state.p&=~(FLAG_V|FLAG_C); if(~(m_state.a^v)&(m_state.a^s)&0x80) m_state.p|=FLAG_V; if(s>0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)s; updateNZ(m_state.a); m_state.cycles++; break; }
        case 0xE9: { uint8_t v=read8(m_state.pc++); uint16_t d=(uint16_t)m_state.a-v-((m_state.p&FLAG_C)?0:1); m_state.p&=~(FLAG_V|FLAG_C); if((m_state.a^v)&(m_state.a^d)&0x80) m_state.p|=FLAG_V; if(d<=0xFF) m_state.p|=FLAG_C; m_state.a=(uint8_t)d; updateNZ(m_state.a); m_state.cycles++; break; }
        case 0xC9: { uint8_t v=read8(m_state.pc++); uint16_t d=(uint16_t)m_state.a-v; m_state.p&=~FLAG_C; if(d<=0xFF) m_state.p|=FLAG_C; updateNZ((uint8_t)d); m_state.cycles++; break; }

        // Logical Immediate
        case 0x29: m_state.a &= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break; // AND
        case 0x09: m_state.a |= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break; // ORA
        case 0x49: m_state.a ^= read8(m_state.pc++); updateNZ(m_state.a); m_state.cycles++; break; // EOR

        // Transfers
        case 0xAA: m_state.x = m_state.a; updateNZ(m_state.x); break; // TAX
        case 0x8A: m_state.a = m_state.x; updateNZ(m_state.a); break; // TXA
        case 0xA8: m_state.y = m_state.a; updateNZ(m_state.y); break; // TAY
        case 0x98: m_state.a = m_state.y; updateNZ(m_state.a); break; // TYA
        case 0x4B: m_state.z = m_state.a; updateNZ(m_state.z); break; // TAZ
        case 0x6B: m_state.a = m_state.z; updateNZ(m_state.a); break; // TZA
        case 0x5B: m_state.b = m_state.a; updateNZ(m_state.b); break; // TAB
        case 0x7B: m_state.a = m_state.b; updateNZ(m_state.a); break; // TBA

        // Increments
        case 0xE8: m_state.x++; updateNZ(m_state.x); break; // INX
        case 0xC8: m_state.y++; updateNZ(m_state.y); break; // INY
        case 0x1A: m_state.a++; updateNZ(m_state.a); break; // INA
        
        // Flags
        case 0x18: m_state.p &= ~FLAG_C; break; // CLC
        case 0x38: m_state.p |=  FLAG_C; break; // SEC

        // Flow
        case 0x4C: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); m_state.pc=(h<<8)|l; m_state.cycles+=2; break; }
        case 0x20: { uint16_t l=read8(m_state.pc++); uint16_t h=read8(m_state.pc++); m_state.cycles+=5; uint16_t r=m_state.pc-1; write8(m_state.sp--, r>>8); write8(m_state.sp--, r&0xFF); m_state.pc=(h<<8)|l; break; }
        case 0x60: { uint8_t l=read8(++m_state.sp); uint8_t h=read8(++m_state.sp); m_state.pc=((h<<8)|l)+1; m_state.cycles+=5; break; }
        case 0xF0: { int8_t r=(int8_t)read8(m_state.pc++); m_state.cycles++; if(m_state.p&FLAG_Z){ m_state.pc+=r; m_state.cycles++; } break; } // BEQ
        case 0xD0: { int8_t r=(int8_t)read8(m_state.pc++); m_state.cycles++; if(!(m_state.p&FLAG_Z)){ m_state.pc+=r; m_state.cycles++; } break; } // BNE
        
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
bool MOS45GS02::isProgramEnd(IBus* bus) { return m_state.haltLine; }
int MOS45GS02::writeCallHarness(IBus* bus, uint32_t scratch, uint32_t routineAddr) {
    bus->write8(scratch, 0x20); bus->write8(scratch+1, routineAddr&0xFF); bus->write8(scratch+2, routineAddr>>8); bus->write8(scratch+3, 0x00); return 4;
}
