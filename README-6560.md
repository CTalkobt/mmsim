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

### 2.3 Sound Synthesis
- **Four Channels**: Three square-wave tone generators and one pseudo-random noise channel.
- **Tone oscillators** (`$900A`–`$900C`): Each register's bit 7 enables the voice; bits 6–0 set the
  frequency divisor F (0–127). Half-period in CPU cycles = `PRESCALER × (128 − F)` where
  PRESCALER = 128 / 64 / 32 for voices 1–3, giving ranges ≈ 31–3995 Hz / 62–7990 Hz / 125–15980 Hz.
- **Noise channel** (`$900D`): Same enable/frequency register format; drives a Galois 16-bit LFSR
  (polynomial 0xB400) clocked at half-period = `16 × (128 − F)`.
- **Volume** (`$900E` bits 3–0): Single master volume 0–15 applied after mixing all four channels.
- **Output interface**: Implements `IAudioOutput`; the host calls `pullSamples()` to drain the
  internal 8192-sample ring buffer that `tick()` fills at the configured sample rate (default 44100 Hz).

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
