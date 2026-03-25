# mmemu-plugin-kbd-vic20: VIC-20 Keyboard Matrix Implementation

This document describes the Keyboard Matrix implementation for the VIC-20 machine on the mmemu platform.

---

## 1. Intent
Provides a standard 8x8 keyboard matrix simulation specifically for the Commodore VIC-20. It allows both physical keyboard mapping (in the GUI) and programmatic keystroke injection (via CLI and MCP).

---

## 2. Functionality

### 2.1 Matrix Simulation
- **8x8 Grid**: Tracks 64 keys across 8 rows and 8 columns.
- **Port Interfaces**: Provides two `IPortDevice` interfaces via `getPort(index)`:
    - **Column Port (Index 0)**: Wired to a column output port (e.g. VIA #1 Port A).
    - **Row Port (Index 1)**: Wired to a row input port (e.g. VIA #2 Port B).
- **Masking**: Correctly simulates the active-low logic of the Commodore keyboard hardware.

### 2.2 Host Interface (`IKeyboardDevice`)
- **Key Injection**: `keyDown(row, col)` and `keyUp(row, col)` for direct matrix access.
- **Symbolic Mapping**: `pressKeyByName(name, down)` allows using human-readable names like "SPACE", "RETURN", or "A".

---

## 3. Dependencies
- **Depends on**: None (Core mmemu interfaces only).
- **Required by**: [VIC-20 Machine Plugin](README-VIC20.md).

---

## 4. Implementation Details
- **Source Location**: `src/plugins/devices/kbd_vic20/`
- **Library**: `lib/internal/libdeviceKbdVic20.a`
- **Plugin**: `lib/mmemu-plugin-kbd-vic20.so`
- **Class**: `KbdVic20`
- **Registration Name**: `"kbd_vic20"`
