# mmemu-plugin-gtia: Atari GTIA Implementation

This document describes the GTIA (Graphics Television Interface Adaptor) implementation for the mmemu platform.

---

## 1. Intent
GTIA is the secondary video chip in the Atari 8-bit family. It receives digital video data from ANTIC and converts it into a color signal. It also handles player-missile graphics (PMG), collision detection, and console switches.

---

## 2. Functionality

### 2.1 Color Generation
- **Palette**: Implements the standard 256-color Atari palette (16 hues x 16 luminances).
- **RGBA Mapping**: Provides `GTIA::getRGBA(index)` for easy conversion to host-side color formats.

### 2.2 Player-Missile Graphics (PMG)
- **Positioning**: Supports horizontal positioning for 4 players and 4 missiles ($D000–$D007).
- **Size Control**: Supports size control (1x, 2x, 4x width) for players and missiles ($D008–$D00C).
- **Graphics Data**: Registers for direct graphics data injection ($D00D–$D011).

### 2.3 Collision Detection
- **Hardware Latches**: 16 read-only registers ($D000–$D00F) record collisions between players, missiles, and playfield.
- **HITCLR**: Writing to $D01E clears all collision latches.

### 2.4 Console Switches
- **CONSOL ($D01F)**: Reads the state of Start, Select, and Option buttons (active low).

---

## 3. Register Map ($D000–$D01F)

GTIA uses overlapping addresses for read and write operations.

### Write-Only Registers

| Offset | Symbol | Description |
|--------|--------|-------------|
| $00-$03 | HPOSP0-3 | Horizontal Position Players 0-3 |
| $04-$07 | HPOSM0-3 | Horizontal Position Missiles 0-3 |
| $08-$0B | SIZEP0-3 | Size Players 0-3 |
| $0C | SIZEM | Size Missiles |
| $0D-$10 | GRAFP0-3 | Graphics Data Players 0-3 |
| $11 | GRAFM | Graphics Data Missiles |
| $12-$15 | COLPM0-3 | Color Players 0-3 |
| $16-$19 | COLPF0-3 | Color Playfields 0-3 |
| $1A | COLBK | Color Background |
| $1B | PRIOR | Priority Select |
| $1C | VDELAY | Vertical Delay |
| $1D | GRACTL | Graphics Control |
| $1E | HITCLR | Clear Collisions |
| $1F | CONSOL | Console Speaker / Switches |

### Read-Only Registers

| Offset | Symbol | Description |
|--------|--------|-------------|
| $00-$03 | M0-3PF | Missile to Playfield Collisions |
| $04-$07 | P0-3PF | Player to Playfield Collisions |
| $08-$0B | M0-3PL | Missile to Player Collisions |
| $0C-$0F | P0-3PL | Player to Player Collisions |
| $10-$13 | TRIG0-3 | Joystick Triggers |
| $14 | PAL | PAL/NTSC Status |
| $1F | CONSOL | Console Switches |

---

## 4. Dependencies
- **Depends on**: Core mmemu interfaces.
- **Required by**: Atari Machine Plugins (Phase 26.4).

---

## 5. Implementation Details
- **Source Location**: `src/plugins/devices/gtia/`
- **Plugin**: `lib/mmemu-plugin-gtia.so`
- **Class**: `GTIA`
- **Registration Name**: `"GTIA"`
