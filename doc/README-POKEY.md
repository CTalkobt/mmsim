# mmemu-plugin-pokey: Atari POKEY Implementation

This document describes the POKEY (Potentiometer and Keyboard Encoder) implementation for the mmemu platform.

---

## 1. Intent
POKEY is a multi-purpose chip used in the Atari 8-bit family and many arcade games. It handles audio synthesis, interval timers, keyboard scanning, and serial I/O (SIO).

---

## 2. Functionality

### 2.1 Audio Subsystem
- **Four Channels**: Independent frequency and control for each of the four audio channels.
- **Poly Counters (Distortion)**: Implements the 4-bit, 5-bit, 9-bit, and 17-bit polynomial noise generators.
- **Volume Only Mode**: Support for direct DAC control via the volume-only bit.
- **Clocking**: Supports 1.79MHz, 64kHz, and 15kHz clocking modes (simplified in this version).
- **Joined Channels**: Support for joining Ch1+Ch2 and Ch3+Ch4 for 16-bit frequency resolution.

### 2.2 Timers
- **Interval Timers**: Channels 1, 2, and 4 can act as interval timers.
- **Interrupts**: IRQ generation on timer underflow ($D20E/$D20F).

### 2.3 Keyboard and I/O
- **Keyboard Scan**: Skeleton for serial keyboard matrix scanning.
- **RANDOM**: $D20B returns bits 8-15 of the 17-bit poly counter.
- **ALLPOT**: $D209 returns pot scan status (simplified to $FF).

### 2.4 SIO (Serial I/O)
- Skeleton for serial port control and status ($D20D, $D20F).

---

## 3. Register Map ($D200–$D20F)

| Offset | Symbol | Access | Description |
|--------|--------|--------|-------------|
| $00 | AUDF1 | W | Audio Frequency Ch 1 |
| $01 | AUDC1 | W | Audio Control Ch 1 |
| $02 | AUDF2 | W | Audio Frequency Ch 2 |
| $03 | AUDC2 | W | Audio Control Ch 2 |
| $04 | AUDF3 | W | Audio Frequency Ch 3 |
| $05 | AUDC3 | W | Audio Control Ch 3 |
| $06 | AUDF4 | W | Audio Frequency Ch 4 |
| $07 | AUDC4 | W | Audio Control Ch 4 |
| $08 | AUDCTL | W | Audio Control (Global) |
| $09 | STIMER | W | Start Timers |
| $09 | ALLPOT | R | Potentiometer Status |
| $0A | SKRES | W | Reset Serial Status |
| $0A | KBCODE | R | Keyboard Code |
| $0B | POTGO | W | Start Pot Scan |
| $0B | RANDOM | R | Random Number (Poly17) |
| $0D | SEROUT | W | Serial Output |
| $0D | SERIN | R | Serial Input |
| $0E | IRQEN | W | IRQ Enable |
| $0E | IRQST | R | IRQ Status |
| $0F | SKCTL | W | Serial Control |
| $0F | SKSTAT | R | Serial Status |

---

## 4. Dependencies
- **Depends on**: `IAudioOutput` (from `libdevices`)
- **Required by**: Atari Machine Plugins (Phase 26.4).

---

## 5. Implementation Details
- **Source Location**: `src/plugins/devices/pokey/`
- **Plugin**: `lib/mmemu-plugin-pokey.so`
- **Class**: `POKEY`
- **Registration Name**: `"POKEY"`
