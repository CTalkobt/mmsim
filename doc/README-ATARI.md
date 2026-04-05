# mmemu-plugin-atari: Atari 8-bit Family Machine Implementation

This document describes the Atari 8-bit Family machine implementation for the mmemu platform.

---

## 1. Intent
The Atari machine plugin acts as the composition layer that integrates individual hardware components (CPU, memory, video, I/O) into a coherent, bootable simulation of the classic Atari 8-bit home computers (400, 800, XL, XE).

---

## 2. Functionality

### 2.1 Hardware Composition
- **CPU**: Instantiates a MOS 6502 core.
- **DMA/Video**: Instantiates the ANTIC DMA engine and display controller.
- **Video/Color**: Instantiates the GTIA color generation and PMG chip.
- **Audio/Timers**: Instantiates the POKEY audio, timers, and keyboard/SIO chip.
- **I/O/Banking**: Instantiates a MOS 6520 PIA for joystick ports and memory banking (XL/XE).
- **Memory**: Configures a 64 KB address space with restricted I/O windows and dynamic ROM/RAM banking.

### 2.2 Memory Map (Atari 800XL)
| Range | Size | Description |
|-------|------|-------------|
| $0000-$9FFF | 40 KB | Main User RAM |
| $A000-$BFFF | 8 KB  | BASIC ROM (bankable via PIA) |
| $C000-$CFFF | 4 KB  | OS ROM (bankable via PIA) |
| $D000-$D01F | 32 B  | GTIA Registers |
| $D200-$D20F | 16 B  | POKEY Registers |
| $D300-$D303 | 4 B   | PIA Registers (swapped RS pins) |
| $D400-$D40F | 16 B  | ANTIC Registers |
| $D800-$FFFF | 10 KB | OS ROM (bankable via PIA) |

---

## 3. Dependencies
This plugin is a **compositional node** and has heavy dependencies on other plugins:
- **Depends on**:
    - [mmemu-plugin-6502](README-6502.md): Provides the execution core.
    - [mmemu-plugin-antic](README-ANTIC.md): Provides the DMA and display list processing.
    - [mmemu-plugin-gtia](README-GTIA.md): Provides color generation and PMG.
    - [mmemu-plugin-pokey](README-POKEY.md): Provides audio and timers.
    - [mmemu-plugin-6520](README-6520.md): Provides I/O and XL/XE banking.

---

## 4. Implementation Details
- **Source Location**: `src/plugins/machines/atari/`
- **Plugin**: `lib/mmemu-plugin-atari.so`
- **Registration Name**: `"atari800xl"`
