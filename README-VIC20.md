# mmemu-plugin-vic20: Commodore VIC-20 Machine Implementation

This document describes the Commodore VIC-20 machine implementation for the mmemu platform.

---

## 1. Intent
The VIC-20 machine plugin acts as the composition layer that integrates individual hardware components (CPU, memory, video, I/O) into a coherent, bootable simulation of the classic Commodore VIC-20 home computer.

---

## 2. Functionality

### 2.1 Hardware Composition
- **CPU**: Instantiates a MOS 6502 core.
- **Video**: Instantiates a MOS 6560 VIC-I chip.
- **I/O**: Instantiates two MOS 6522 VIA chips.
- **Memory**: Configures a 64 KB address space with appropriate mappings for RAM, ROM, and I/O windows.

### 2.2 Memory Map
| Range | Size | Description |
|-------|------|-------------|
| $0000-$03FF | 1 KB | Zero Page, Stack, System Vectors |
| $0400-$0FFF | 3 KB | Expansion Area (User RAM) |
| $1000-$1DFF | 3.5KB| Main User RAM |
| $1E00-$1FFF | 0.5KB| Screen RAM (Default) |
| $8000-$8FFF | 4 KB | Character ROM |
| $9000-$900F | 16 B | VIC-I Registers |
| $9110-$911F | 16 B | VIA #1 Registers |
| $9120-$912F | 16 B | VIA #2 Registers |
| $9400-$97FF | 1 KB | Color RAM |
| $C000-$DFFF | 8 KB | BASIC ROM |
| $E000-$FFFF | 8 KB | KERNAL ROM |

### 2.3 Scheduler
- Implements a synchronous cycle-stepper that ticks all I/O devices (`tickAll`) in lock-step with CPU instruction execution.

---

## 3. Dependencies
This plugin is a **compositional node** and has heavy dependencies on other plugins:
- **Depends on**:
    - [mmemu-plugin-6502](README-6502.md): Provides the execution core.
    - [mmemu-plugin-6522](README-6522.md): Provides the I/O and timers.
    - [mmemu-plugin-6560](README-6560.md): Provides the video generation.
    - [mmemu-plugin-kbd-vic20](README-KBD-VIC20.md): Provides the keyboard matrix.

---

## 4. Implementation Details
- **Source Location**: `src/plugins/machines/vic20/`
- **Plugin**: `lib/mmemu-plugin-vic20.so`
- **Registration Name**: `"vic20"`
