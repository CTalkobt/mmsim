# mmemu-plugin-45gs02: MOS 45GS02 Implementation (MEGA65)

The 45GS02 is the CPU core of the MEGA65. It extends the 8-bit MOS 6502/65C02 architecture with 65CE02 and 4510-compatible enhancements, along with MEGA65-specific 32-bit "Quad" operations and a 28-bit MMU.

---

## 1. Architecture Overview

### 1.1 Registers

| Register | Name | Width | Description |
|----------|------|-------|-------------|
| **A**    | Accumulator | 8-bit | Primary 8-bit accumulator. |
| **X**    | Index X | 8-bit | Index register. |
| **Y**    | Index Y | 8-bit | Index register. |
| **Z**    | Index Z | 8-bit | Third index register (65CE02). |
| **B**    | Base Page | 8-bit | Replaces Zero Page; points to any 256-byte page. |
| **SP**   | Stack Pointer | 16-bit| Can address any page; wraps at 8-bit if E-flag=1. |
| **PC**   | Program Counter | 16-bit| Address of next instruction. |
| **P**    | Status Flags | 8-bit | NV-BDIZC flags. Bit 3 (E) is the 6502 Emulation flag. |
| **Q**    | Quad | 32-bit| Logical combination of A (LSB), X, Y, Z (MSB). |

### 1.2 Status Flags (P Register)

Same as 6502, but bit 5 is used as the **E (Emulation)** flag in some variations. In this implementation, when **E** is set, the stack pointer behaves like a standard 6502 (fixed at page 1).

---

## 2. Instruction Set Discussion

The 45GS02 supports the full 6502/65C02 sets, plus many extensions.

### 2.1 Standard 6502/65C02 Base
- All standard operations: `LDA`, `STA`, `ADC`, `SBC`, `AND`, `ORA`, `EOR`, `CMP`, `CPX`, `CPY`, `BIT`, `INC`, `DEC`, `ASL`, `LSR`, `ROL`, `ROR`, `JMP`, `JSR`, `RTS`, `RTI`, and all branches.
- **65C02 Extensions**: `STZ` (Store Zero), `BRA` (Branch Always), `PHX/PLX`, `PHY/PLY`, `TSB`/`TRB` (Test and Set/Reset Bits).

### 2.2 65CE02 / 4510 Enhancements
- **New Registers**: `LDZ`, `STZ`, `INZ`, `DEZ`, `PHZ`, `PLZ`, `TSY`, `TYS`.
- **Advanced Addressing**:
  - `(zp),Z`: Indirect indexed via Z.
  - `(d,S),Y`: Stack-relative indirect indexed via Y.
- **16-bit Word Ops**:
  - `INW`: Increment 16-bit word in memory (Zp).
  - `DEW`: Decrement 16-bit word in memory (Zp).
  - `ASW`: Arithmetic Shift Left 16-bit word (Absolute).
  - `ROW`: Rotate Left 16-bit word (Absolute).
- **Long Branches**: 16-bit relative offsets for all conditions (e.g., `LBRA`, `LBEQ`, `LBNE`).

### 2.3 MEGA65 Specific Extensions (45GS02)

#### 32-bit "Quad" Operations
Prefixing an instruction with `$42 $42` (actually double prefix) or using the `LDQ`/`STQ` mnemonics allows treating A, X, Y, and Z as a single 32-bit register.
- **Quad Load/Store**: `LDQ`, `STQ`.
- **Quad Arithmetic**: `ADCQ`, `SBCQ`, `CMPQ`.
- **Quad Logical**: `ANDQ`, `ORAQ`, `EORQ`, `BITQ`.
- **Quad Shifts**: `ASLQ`, `LSRQ`, `ROLQ`, `RORQ`, `ASRQ`.
- **Quad Misc**: `NEGQ`, `INQ`, `DEQ`.

#### Memory Management
- **MAP**: Enables the 28-bit MMU (4 x 16KB slots).
- **EOM**: Disables the MMU (returns to flat 16-bit 64KB mode).

#### Bit Manipulation (RMBx / SMBx / BBRx / BBSx)
- **RMB0-7**: Reset Memory Bit (Zp).
- **SMB0-7**: Set Memory Bit (Zp).
- **BBR0-7**: Branch if Bit Reset (Zp, Rel).
- **BBS0-7**: Branch if Bit Set (Zp, Rel).

#### Miscellaneous
- **NEG**: Two's complement negation of Accumulator.
- **RTN**: Return from subroutine and add immediate to SP (Cleanup stack).
- **BSR**: Branch to Subroutine (8-bit or 16-bit relative).
- **PHW**: Push 16-bit Word (Immediate or Absolute) onto stack.

---

## 3. Implementation Specifics

### 3.1 32-bit Indirect Addressing
Prefixing an indirect instruction (like `LDA ($zp),Z`) with `$EA` enables 32-bit pointers. This implementation treats them as **28-bit physical addresses**, bypassing the MAP MMU and addressing the full 256MB MEGA65 space directly.
Syntax in disassembly: `LDA [$zp],Z`.

### 3.2 MMU (MAP)
When `MAP` is active, the 64KB virtual space is split into four 16KB slots:
- Slot 0: `$0000–$3FFF`
- Slot 1: `$4000–$7FFF`
- Slot 2: `$8000–$BFFF`
- Slot 3: `$C000–$FFFF`

Physical Address = `(map[slot] << 8) | (vaddr & 0x3FFF)`.

---

## 4. Usage in mmsim

The plugin is registered as `"45gs02"`.

### Example Machine Configuration
```json
{
  "cpu": {
    "type": "45gs02",
    "dataBus": "system",
    "codeBus": "system"
  }
}
```

### Debugging
The 45GS02 implementation includes a "Hypervisor Trap" at `$D6CF`. Writing `$42` to this address will immediately halt the simulation, which is useful for finishing automated test runs.
