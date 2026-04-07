# mmemu-plugin-6502: MOS 6502 Processor Implementation

This document provides a comprehensive discussion of the 6502 implementation within the mmemu ecosystem. This component is delivered as a dynamically loadable plugin (`mmemu-plugin-6502.so`).

---

## 1. Architecture Overview

The MOS 6502 is an 8-bit microprocessor with a 16-bit address bus, capable of addressing up to 64 KB of memory. It uses a "Little Endian" format for multi-byte values in memory.

### 1.1 Registers
The implementation exposes the standard 6502 register set to the host:

| Register | Name | Width | Description |
|----------|------|-------|-------------|
| **A**    | Accumulator | 8-bit | Primary register for arithmetic and logic. |
| **X**    | Index X | 8-bit | Used for indexing and as a counter. |
| **Y**    | Index Y | 8-bit | Used for indexing and as a counter. |
| **SP**   | Stack Pointer | 8-bit | Points to the next free location in the hardware stack (page 1: $0100-$01FF). |
| **PC**   | Program Counter | 16-bit| Holds the address of the next instruction to execute. |
| **P**    | Status Register | 8-bit | Individual flag bits representing the CPU state. |

### 1.2 Status Flags (P Register)
- **C (Carry)**: Set if the last operation caused an overflow from bit 7 or a borrow during subtraction.
- **Z (Zero)**: Set if the result of the last operation was zero.
- **I (IRQ Disable)**: When set, prevents the CPU from responding to maskable interrupts.
- **D (Decimal Mode)**: When set, arithmetic operations are performed in Binary Coded Decimal (BCD).
- **B (Break Command)**: Set when a `BRK` instruction is executed and pushed onto the stack.
- **U (Unused)**: Always reads as 1 on the 6502.
- **V (Overflow)**: Set if an arithmetic operation resulted in a signed overflow.
- **N (Negative)**: Set if the most significant bit (bit 7) of the result was set.

---

## 2. Instruction Set Discussion

The implementation supports the full NMOS 6502 instruction set, including both documented and undocumented opcodes.

### 2.1 Documented Opcodes (Official)

#### Load/Store Operations
- **LDA**: Load Accumulator with memory. Sets N and Z flags.
- **LDX**: Load Index X with memory. Sets N and Z flags.
- **LDY**: Load Index Y with memory. Sets N and Z flags.
- **STA**: Store Accumulator in memory.
- **STX**: Store Index X in memory.
- **STY**: Store Index Y in memory.

#### Register Transfers
- **TAX**: Transfer Accumulator to X.
- **TAY**: Transfer Accumulator to Y.
- **TXA**: Transfer X to Accumulator.
- **TYA**: Transfer Y to Accumulator.

#### Stack Operations
- **TSX**: Transfer Stack Pointer to X.
- **TXS**: Transfer X to Stack Pointer.
- **PHA**: Push Accumulator onto stack.
- **PHP**: Push Processor Status onto stack.
- **PLA**: Pull Accumulator from stack.
- **PLP**: Pull Processor Status from stack.

#### Logical Operations
- **AND**: Logical AND Accumulator with memory.
- **EOR**: Exclusive OR Accumulator with memory.
- **ORA**: Logical OR Accumulator with memory.
- **BIT**: Test bits in memory with Accumulator. Affects N, V, and Z flags.

#### Arithmetic Operations
- **ADC**: Add memory to Accumulator with Carry. Supports Decimal mode (BCD).
- **SBC**: Subtract memory from Accumulator with Borrow. Supports Decimal mode (BCD).
- **CMP**: Compare Accumulator with memory.
- **CPX**: Compare Index X with memory.
- **CPY**: Compare Index Y with memory.

#### Increments/Decrements
- **INC**: Increment memory value.
- **INX**: Increment Index X.
- **INY**: Increment Index Y.
- **DEC**: Decrement memory value.
- **DEX**: Decrement Index X.
- **DEY**: Decrement Index Y.

#### Shifts/Rotates
- **ASL**: Arithmetic Shift Left. Shifts bit 7 into Carry and bit 0 becomes 0.
- **LSR**: Logical Shift Right. Shifts bit 0 into Carry and bit 7 becomes 0.
- **ROL**: Rotate Left. Shifts bit 7 into Carry and Carry into bit 0.
- **ROR**: Rotate Right. Shifts bit 0 into Carry and Carry into bit 7.

#### Control Flow
- **JMP**: Jump to a new location.
- **JSR**: Jump to a new location, saving the return address on the stack.
- **RTS**: Return from subroutine.
- **RTI**: Return from interrupt.
- **BCC/BCS**: Branch if Carry Clear/Set.
- **BEQ/BNE**: Branch if Zero Set/Clear (Equal/Not Equal).
- **BMI/BPL**: Branch if Negative Set/Clear (Minus/Plus).
- **BVC/BVS**: Branch if Overflow Clear/Set.

#### Status Flag Changes
- **CLC/SEC**: Clear/Set Carry flag.
- **CLD/SED**: Clear/Set Decimal flag.
- **CLI/SEI**: Clear/Set Interrupt Disable flag.
- **CLV**: Clear Overflow flag.

#### System Functions
- **BRK**: Force an interrupt. Pushes PC and Status (with Break bit set) to stack and jumps to IRQ vector.
- **NOP**: No Operation.

---

### 2.2 Undocumented Opcodes (Illegal)

These instructions are not part of the official specification but exist due to the way the NMOS 6502 hardware is wired. They are frequently used in Commodore software.

- **LAX**: (LDA + LDX) Loads both Accumulator and X register from memory.
- **SAX**: (STA + STX) Stores the logical AND of A and X into memory.
- **SLO**: (ASL + ORA) Shifts memory left, then ORs the result into A.
- **RLA**: (ROL + AND) Rotates memory left, then ANDs the result into A.
- **SRE**: (LSR + EOR) Shifts memory right, then EORs the result into A.
- **RRA**: (ROR + ADC) Rotates memory right, then adds the result to A with Carry.
- **DCP**: (DEC + CMP) Decrements memory, then compares the result with A.
- **ISC**: (INC + SBC) Increments memory, then subtracts the result from A.
- **ANC**: ANDs Accumulator with immediate value, then copies N flag into Carry.
- **ALR**: ANDs Accumulator with immediate value, then performs LSR on the result.
- **ARR**: ANDs Accumulator with immediate value, then performs ROR on the result.
- **XAA**: Transfers X to A, then ANDs A with immediate value (result can be unstable on real hardware).
- **AXS**: ANDs A and X, then subtracts immediate value from the result and stores it in X.
- **LAS**: ANDs memory with stack pointer, then stores result in A, X, and SP.
- **AHX/TAS/SHY/SHX**: Highly unstable opcodes that perform complex AND/Store operations (implemented as stubs).
- **KIL**: Crashes the CPU (Jam).

---

## 3. Implementation Specifics

### 3.1 Cycle Accuracy
Every instruction is modeled with its correct base cycle count.
- **Page Crossing**: Indexed addressing modes (`ABS,X`, `ABS,Y`, `(BP),Y`) add 1 cycle if a page boundary is crossed.
- **Branch Taken**: Branch instructions add 1 cycle if the condition is met, and 1 additional cycle if the target is on a different page.

### 3.2 The Indirect JMP Bug
The faithfully reproduced bug: `JMP ($xxFF)` fetches the high byte of the target address from `$xx00` instead of `$(xx+1)00`.

---

## 4. Toolchain Support

### 4.1 Disassembler
Decodes all 256 opcode bytes (including all illegal/undocumented instructions) into standard assembly syntax. It provides control-flow metadata (calls, returns, branches) for the debugger.

### 4.2 Native Mini-Assembler
Supports full standard 6502 assembly syntax for both the CLI `.` (direct execute) and `asm` (interactive mode) commands.
- **Mnemonics**: All 56 standard opcodes plus common undocumented ones (`SLO`, `RLA`, `SRE`, `RRA`, `SAX`, `LAX`, `DCP`, `ISC`).
- **Addressing Modes**: Complete support for Immediate, Zero Page, Absolute, Indexed (X and Y), Indirect, and Relative (including automatic branch offset calculation).
- **Flexibility**: Handles both hex (`$`) and decimal values.

---

## 5. Implementation Details
- **Source Location**: `src/plugins/6502/`
- **Class**: `MOS6502`
- **ABI Compliance**: Exports `mmemuPluginInit`.

---

## 6. MOS 6510 Variant

The same plugin also provides the **MOS 6510** — the CPU used in the Commodore 64. It extends `MOS6502` with a built-in 6-bit parallel I/O port at addresses $0000 (DDR) and $0001 (DATA) that drives the C64 PLA banking lines. See [README-6510.md](README-6510.md) for full details.

| Registration Name | Class | Machine Preset |
|-------------------|-------|----------------|
| `"6502"` | `MOS6502` | `"raw6502"` |
| `"6510"` | `MOS6510` | `"raw6510"` |
