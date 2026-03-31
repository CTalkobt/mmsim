# mmemu-plugin-crtc6545: MOS 6545/6845 CRTC Implementation

The MOS 6545 / Motorola 6845 Cathode Ray Tube Controller (CRTC) is a programmable controller used to generate the timing signals and memory addresses required for video display in many 8-bit computers, including the later Commodore PET models (4000 and 8000 series). It is implemented as an `IOHandler` plugin within `mmemu-plugin-crtc6545.so`.

---

## 1. Intent

The CRTC manages video memory access and sync timing. It provides internal registers to configure screen geometry (columns, rows, character size) and handles the generation of memory addresses for fetching character data and the row address within each character glyph.

---

## 2. Functionality

### 2.1 Register Interface

The CRTC is accessed via two memory-mapped locations:
- **Address Register** (Base + 0): Used to select the internal register to be accessed.
- **Data Register** (Base + 1): Used to read from or write to the selected internal register.

### 2.2 Internal Registers (Partial List)

| Register | Name | Description |
|----------|------|-------------|
| R0 | Horizontal Total | Total number of characters in a horizontal line, including sync. |
| R1 | Horizontal Displayed | Number of characters displayed per line (e.g., 40 or 80). |
| R2 | Horizontal Sync Position | Character position where horizontal sync begins. |
| R3 | Sync Widths | Defines horizontal and vertical sync pulse widths. |
| R4 | Vertical Total | Total number of character rows in a frame. |
| R5 | Vertical Total Adjust | Number of additional scanlines to adjust vertical timing. |
| R6 | Vertical Displayed | Number of character rows displayed per frame (typically 25). |
| R7 | Vertical Sync Position | Row position where vertical sync begins. |
| R9 | Max Scanline Address | Number of scanlines per character (typically 7 or 9 for 8x8 or 8x10). |
| R12/R13 | Start Address (H/L) | 14-bit start address in video RAM for the first character. |
| R14/R15 | Cursor Address (H/L) | 14-bit address for the hardware cursor. |

### 2.3 Timing and Address Generation

`tick(cycles)` advances the internal counters:
- **hCount**: Horizontal character counter.
- **vCount**: Vertical character row counter.
- **ra**: Row address (scanline within the current character row).
- **ma**: Memory address (offset into video RAM).

The status signals `hSync`, `vSync`, and `dispEnable` (display enable) are updated based on these counters and register settings.

---

## 3. Register Map (Host View)

Typically mapped at `$E880` in Commodore PET systems:

| Address | Symbol | Description |
|---------|--------|-------------|
| $E880 | ADDR_REG | Select internal register (write); Status (read) |
| $E881 | DATA_REG | Read/Write selected register |

---

## 4. Dependencies

- **Required by**: PET Video Subsystem (Phase 12.3)

---

## 5. Implementation Details

- **Source Location**: `src/plugins/devices/crtc6545/main/crtc6545.h/cpp`
- **Plugin**: `lib/mmemu-plugin-crtc6545.so`
- **Class**: `CRTC6545 : public IOHandler`
- **Registration Name**: `"6545"`
- **Base Address**: `$E880` (Standard PET location)
- **Address Mask**: `0x0001` (2-byte window)
