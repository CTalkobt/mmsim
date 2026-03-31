# mmemu-plugin-antic: Atari ANTIC Implementation

This document describes the ANTIC (Alpha-Numeric Television Interface Controller) implementation for the mmemu platform.

---

## 1. Intent
ANTIC is the primary DMA engine and display controller for the Atari 8-bit family (400/800/XL/XE). It interprets a "Display List" (a simple machine-code-like program for the screen) and generates timing and data for the GTIA chip.

---

## 2. Functionality

### 2.1 Display List Interpreter
- **Instruction Fetch**: ANTIC steals cycles from the CPU to fetch instructions from memory.
- **LMS (Load Memory Scan)**: Supports the LMS bit to set the memory scan counter for a mode line.
- **DLI (Display List Interrupt)**: Fires an NMI when the DLI bit is set in an instruction and NMIEN is enabled.
- **JMP/JSR**: Supports jumping to a new Display List address.

### 2.2 DMA Timing
- **Cycle Stealing**: Uses the `HALT` line to pause the CPU during DMA fetches.
- **Scanline Timing**: Accurate 114-cycle per scanline and 262-line per frame timing (NTSC).
- **VCOUNT**: Implements the vertical line counter ($D40B), which returns 1/2 the current line number.

### 2.3 Interrupts (NMI)
- **VBI (Vertical Blank Interrupt)**: Triggered at the start of the vertical blank period (line 248+).
- **DLI (Display List Interrupt)**: Triggered by the DLI bit in Display List instructions.
- **NMIST/NMIRES**: Correct status and reset behavior for NMI management.

### 2.4 Synchronization
- **WSYNC (Wait for Horizontal Sync)**: Writing to $D40A halts the CPU until the start of the next scanline.

---

## 3. Register Map ($D400–$D40F)

| Offset | Symbol | Access | Description |
|--------|--------|--------|-------------|
| $00 | DMACTL | W | DMA Control |
| $01 | CHACTL | W | Character Control |
| $02 | DLISTL | W | Display List Pointer Low |
| $03 | DLISTH | W | Display List Pointer High |
| $04 | HSCROL | W | Horizontal Scroll |
| $05 | VSCROL | W | Vertical Scroll |
| $07 | PMBASE | W | Player-Missile Base Address |
| $09 | CHBASE | W | Character Base Address |
| $0A | WSYNC | W | Wait for Horizontal Sync |
| $0B | VCOUNT | R | Vertical Line Counter |
| $0E | NMIEN | W | NMI Enable |
| $0F | NMIST | R | NMI Status |
| $0F | NMIRES | W | NMI Reset |

---

## 4. Dependencies
- **Depends on**: `ICore` (for `HALT` signal), `ISignalLine` (for `NMI`).
- **Required by**: Atari Machine Plugins (Phase 26.4).

---

## 5. Implementation Details
- **Source Location**: `src/plugins/devices/antic/`
- **Plugin**: `lib/mmemu-plugin-antic.so`
- **Class**: `Antic`
- **Registration Name**: `"ANTIC"`
