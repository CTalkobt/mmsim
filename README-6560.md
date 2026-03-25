# mmemu-plugin-vic6560: MOS 6560 VIC-I Implementation

This document describes the MOS 6560 (NTSC) / 6561 (PAL) Video Interface Chip implementation for the mmemu platform.

---

## 1. Intent
The VIC-I is the primary video and sound generator for the Commodore VIC-20. It handles character-based text and graphics, light pen inputs, and simple four-channel sound synthesis.

---

## 2. Functionality

### 2.1 Video Rendering
- **Standard Interface**: Implements the `IVideoOutput` interface for universal display compatibility.
- **Character Modes**: Supports standard (8x8) and double-height (8x16) character modes.
- **Multi-color**: Full support for 6502-style multi-color character rendering (2 bits per pixel).
- **Geometry**: Supports programmable screen columns, rows, and origin.

### 2.2 Memory Access
- **Side-effect Free**: Uses `IBus::peek8()` for all background rendering tasks to avoid triggering I/O side effects in the machine.
- **Dynamic Addressing**: Respects VIC registers for Screen RAM and Character Generator base addresses.

### 2.3 Sound Simulation
- **Four Channels**: Three square-wave generators and one noise channel.
- **Simplified Model**: Currently implements an amplitude model; waveform synthesis is a planned upgrade.

---

## 3. Dependencies
This plugin is a **leaf node** in the plugin hierarchy:
- **Depends on**: None (Core mmemu interfaces only).
- **Required by**: [VIC-20 Machine Plugin](README-VIC20.md).

---

## 4. Implementation Details
- **Source Location**: `src/plugins/devices/vic6560/`
- **Library**: `lib/internal/libdeviceVIC6560.a`
- **Plugin**: `lib/mmemu-plugin-vic6560.so`
- **Class**: `VIC6560`
- **Registration Name**: `"6560"`
