# POKEY Chip Implementation Plan (mmsim)

The POKEY (Pot Keyboard Integrated Circuit) is a versatile custom LSI chip in the Atari 8-bit architecture. This document outlines the roadmap for transitioning the current stub implementation into a fully functional emulator component.

## 1. Functional Overview
POKEY handles five primary system responsibilities:
1.  **Audio Generation:** 4 channels of semi-independent square wave/noise audio.
2.  **Timers:** 3 high-resolution timers based on audio dividers.
3.  **Keyboard Scanning:** Debouncing and matrix scanning.
4.  **Serial I/O (SIO):** UART for communication with disk drives, printers, etc.
5.  **Potentiometer Reading:** 8 A/D converters for paddle controllers.
6.  **Random Number Generation:** 17-bit and 9-bit polynomial counters.

---

## 2. Serial I/O (SIO) Implementation
The SIO is currently the primary blocker for OS boot completion.

### UART State Machine
- **Registers:** `SEROUT` ($D20D write), `SERIN` ($D20D read), `SKSTAT` ($D20F read), `SKCTL` ($D20F write).
- **Functionality:** 
    - Implement a shift register model for serial transmission and reception.
    - Support asynchronous mode (standard for SIO) and synchronous mode.
    - Baud rate must be driven by POKEY Timers 3 and 4 (or a fixed 1.79MHz divider).

### External Peripheral Simulation
To allow the OS to boot, we must simulate the response of an Atari Disk Drive (D1:):
- **Handshake Lines:** 
    - **Command:** (Driven by PIA Port A) Active low signals start of command frame.
    - **Data Out:** Data from POKEY to device.
    - **Data In:** Data from device to POKEY.
- **Boot Protocol:**
    1. OS asserts COMMAND.
    2. OS sends 5-byte command frame (Device, Command, Aux1, Aux2, Checksum).
    3. Emulator intercepts this frame.
    4. If command is "Get Status" or "Read Sector 1", emulator must send back:
        - ACK byte ($41)
        - Data frame (if Read)
        - Checksum byte
- **Format Support:** Integrate `libtoolchain` to parse `.atr` and `.xfd` disk images.

---

## 3. Interrupt System (IRQ)
POKEY manages the majority of system IRQs via `IRQEN` ($D20E) and `IRQST` ($D20E).

- **Events to Implement:**
    - Timer 1, 2, 4 underflow.
    - Serial Input Data Ready.
    - Serial Output Needed (Buffer Empty).
    - Serial Transmission Finished.
    - Keyboard Key Pressed.
- **Logic:** `IRQST` bits are active low. Clearing occurs when the corresponding event is acknowledged (usually by writing to `IRQEN`).

---

## 4. Audio Engine
- **Dividers:** 4 channels using 8-bit dividers (can be paired for 16-bit).
- **Poly Counters:** Implement the 4-bit, 5-bit, 9-bit, and 17-bit noise generators.
- **Filtering:** High-pass filter options (Channel 1 -> 3, Channel 2 -> 4).
- **Output:** 1.79MHz sampling downsampled to host audio rate (44.1kHz/48kHz).

---

## 5. Keyboard & Pots
- **Matrix Scanning:** Map host key events to the 6-bit scan code format in `KBCODE` ($D209).
- **Potentiometers:** `ALLPOT` ($D208). Map host mouse/joystick to 0-228 counts.

---

## 6. Implementation Phases

### Phase 1: Minimal SIO & IRQ (High Priority)
- Implement `IRQEN`/`IRQST` basic level-triggering.
- Add "SIO Loopback/Dummy" mode: Automatically ACK any SIO command to prevent OS hangs.
- Correct `SKSTAT` / `SKCTL` register behavior for serial status.

### Phase 2: Audio & Timers
- Implement the 4-channel dividers and `AUDF` / `AUDC` registers.
- Support pairings (16-bit) and 1.79MHz / 64kHz clock switching.
- Hook Timer interrupts into the `IRQST` system.

### Phase 3: Real Disk I/O
- Add ATR file loading.
- Implement the full SIO state machine (Serial -> Command Frame -> Response).
- Support standard 19200 baud and high-speed (US Doubler) protocol.

### Phase 4: Full Audio & Pots
- Poly counters and high-pass filters.
- Real-time resampling for `mmemu-gui`.
