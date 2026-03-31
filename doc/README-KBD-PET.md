# mmemu-plugin-kbd-pet: PET Keyboard Matrix Implementation

The PET Keyboard Matrix plugin simulates the switch matrix hardware used in Commodore PET computers. It supports both the "Graphics" layout (found on 2001/4000 series) and the "Business" layout (found on 8000 series). It is implemented as a specialized `IPortDevice`.

---

## 1. Intent

This plugin translates symbolic key events from the host (GUI/CLI) into raw matrix signals that can be read by the emulated MOS 6520 PIA chip.

---

## 2. Functionality

### 2.1 Matrix Operation

The PET keyboard is organized as an 8x10 matrix (8 columns, 10 rows).
- **Row Selection**: The emulated CPU selects a row by writing a 4-bit value to the PIA1 Port A. This value is decoded by a 74145 BCD-to-Decimal decoder to pull one of the 10 row lines low.
- **Column Reading**: The PIA1 Port B reads the state of the 8 column lines. If a key at the intersection of the selected row and a column is pressed, the corresponding bit in Port B will be low.

### 2.2 Layouts

- **Graphics Layout**: 2001/4000 series style with PETSCII graphic symbols on the keycaps.
- **Business Layout**: 8000 series style with a layout more similar to a typewriter, including a dedicated TAB key and different punctuation placement.

---

## 3. Integration

The keyboard is wired to the **PIA1** device in PET systems:

| PIA1 Port | Direction | Description |
|-----------|-----------|-------------|
| Port A (low 4 bits) | Output | Row Selection (0-9). Bits 4-7 are ignored by the decoder. |
| Port B | Input | Column Data. Bit 0-7 correspond to columns 0-7. |

---

## 4. Dependencies

- **Depends on**: `IPortDevice` (from `libdevices`)
- **Required by**: PET Machine Plugin (Phase 12.5)

---

## 5. Implementation Details

- **Source Location**: `src/plugins/devices/keyboard/main/keyboard_matrix_pet.h/cpp`
- **Class**: `PetKeyboardMatrix : public IPortDevice`
- **Supported Layouts**: `GRAPHICS`, `BUSINESS`
- **Interface**: `keyDown(keyName)`, `keyUp(keyName)`
