# 45GS02 CPU (MEGA65)

The 45GS02 is the heart of the MEGA65, extending the classic MOS 6502 architecture with 65CE02 and 4510-compatible enhancements, plus MEGA65-specific features like 28-bit memory mapping (MAP) and 32-bit Quad operations.

---

## 1. Features

- **Compatibility**: Supports all official 6502 and 65C02 opcodes.
- **Enhanced Registers**:
  - `Z`: Third 8-bit index register.
  - `B`: 8-bit Base Page register (replaces the zero page at `B << 8`).
  - `SP`: 16-bit Stack Pointer.
- **New Addressing Modes**:
  - `(zp),Z`: Indirect indexed via Z register.
  - `(d,S),Y`: Stack-relative indirect indexed.
- **MEGA65 Specifics**:
  - `MAP`: Enables 4-slot 16KB memory mapping.
  - `EOM`: Disables MAP / End of Map.
  - `NEG`: Two's complement negation of the accumulator.
  - `16-bit Word Ops`: `INW`, `DEW`, `ASW`, `ROW` on 16-bit memory words.
  - `32-bit Quad pseudo-register`: Combines A, X, Y, and Z into a single 32-bit logical register for `LDQ`, `STQ`, and arithmetic.

---

## 2. Register Layout

| Name | Width | Flags | Description |
|------|-------|-------|-------------|
| **A** | 8-bit | ACC | Accumulator |
| **X** | 8-bit | - | X Index |
| **Y** | 8-bit | - | Y Index |
| **Z** | 8-bit | - | Z Index (65CE02+) |
| **B** | 8-bit | - | Base Page register (65CE02+) |
| **SP** | 16-bit | SP | Stack Pointer (wraps at 8-bit in 6502 mode) |
| **PC** | 16-bit | PC | Program Counter |
| **P** | 8-bit | STATUS | Status Flags (NV-BDIZC) |

---

## 3. Memory Mapping (MAP)

The `MAP` instruction activates the 45GS02 MMU. When enabled, the 64KB CPU address space is translated into a larger 28-bit physical space using four 16KB slots.

- **Slot 0**: `$0000–$3FFF`
- **Slot 1**: `$4000–$7FFF`
- **Slot 2**: `$8000–$BFFF`
- **Slot 3**: `$C000–$FFFF`

Translation formula: `phys = (map[slot] << 8) | (vaddr & 0x3FFF)`.

---

## 4. Hypervisor Interop

The `mmsim` implementation includes a "Hypervisor Trap" at `$D6CF`. Writing `#$42` to this address triggers a halt, commonly used in test validation to signal program completion.

---

## 5. Usage in mmsim

Select the 45GS02 core in your machine configuration:

```json
"cpu": { "type": "45gs02", "dataBus": "system", "codeBus": "system" }
```
