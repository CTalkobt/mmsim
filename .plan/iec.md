# IEC Serial Bus and Disk Drive Support Architecture

## 1. Overview
The Commodore Serial Bus (IEC) is a bit-serial version of the IEEE-488 bus used in the VIC-20, C64, C128, and Plus/4. The goal of Phase 15 is to provide robust disk drive emulation for Commodore machines, supporting both high-level and low-level emulation strategies.

## 2. Physical Layer and Handshake
The IEC bus uses **open-collector** (wired-OR) logic. Multiple devices can be connected in a "daisy-chain" configuration. A line is logical "true" (1) when pulled low (0V) and "false" (0) when released high (5V).

### 2.1 Signal Lines
- **ATN (Attention):** Driven exclusively by the host. When low, it indicates the host is broadcasting a command (e.g., LISTEN, TALK, UNLISTEN, UNTALK) to all devices.
- **CLK (Clock):** Driven by the Talker to synchronize data bits. Also used for handshaking during addressing.
- **DATA:** Carries the actual data bits. Driven by the Talker or used by the Listener for handshaking.
- **SRQ (Service Request):** Used for fast serial transfers (introduced with the C128).
- **RESET:** Resets all peripherals on the bus.

### 2.2 Standard Handshake (Level 2)
The `VirtualIECBus` state machine must accurately time the standard KERNAL handshake:
1.  **ATN Low:** Host starts a transaction.
2.  **Addressing:** Host sends a command byte (device address + command) using a 1ms handshake.
3.  **Acknowledge:** The addressed device pulls DATA low to signal readiness.
4.  **Bit Transfer:** Data is shifted bit-by-bit (typically 60µs per bit) using CLK/DATA toggling.
5.  **EOI (End Or Identify):** Signaling the last byte of a message by delaying the handshake for >200µs before the byte is sent.

## 3. Emulation Strategy

### 3.1 Level 1: KERNAL HLE (Trapping)
High-level emulation that intercepts KERNAL I/O vectors to provide instant disk access.
- **Implementation:** Uses a `KernalTrap` (ExecutionObserver) to monitor the Program Counter for KERNAL entry points.
- **Vectors to Trap:**
    - `SETLFS ($FFBA)`: Logical file/device/secondary address configuration.
    - `SETNAM ($FFBD)`: Filename specification.
    - `OPEN ($FFC0)`: Opening channels.
    - `CLOSE ($FFC3)`: Closing channels.
    - `LOAD ($FFD5)`: Binary file injection into RAM.
    - `SAVE ($FFD8)`: RAM segment extraction to host file.
    - `CHRIN ($FFCF)` / `CHROUT ($FFD2)`: Byte-stream I/O.
- **Mapping:** Unit 8–11 are mapped to local host folders ("Flat Directory") or disk images.

### 3.2 Level 2: Virtual IEC Bus (Bit-Level)
Protocol-level emulation that simulates the CLK/DATA/ATN signals without emulating the drive's internal hardware.
- **Implementation:** `VirtualIECBus` state machine.
- **Integration:** Handled via `IPortDevice` calls from CIA #2 (C64) or VIA #2 (VIC-20).
- **Compatibility:** Works with software that uses standard KERNAL calls but bit-bangs the bus itself. Breaks most custom "fast-loaders" that rely on drive-side code execution.

### 3.3 Level 3: Low-Level Emulation (LLE)
Full hardware emulation of a disk drive (e.g., Commodore 1541).
- **Components:** Internal 6502 CPU, 2 KB RAM, 16 KB ROM, and two 6522 VIAs.
- **Mechanism:** The drive runs its own KERNAL or fast-loader code, interacting with the bus at the cycle-exact signal level.
- **Status:** Planned for future phases (Phase 30+).

## 4. Disk Image Support
Supported via the `cbm-loader` plugin:
- **.D64:** Standard 170 KB sector-based image (35-40 tracks).
- **.G64:** Raw GCR bit-stream (GCR = Group Code Recording), supporting copy-protection schemes.
- **.PRG:** Raw Commodore program files (loaded via KERNAL HLE or virtual bus stream).
- **.T64:** Tape images formatted as virtual disks.
- **.P00:** Program files with original Commodore metadata headers.

## 5. UI and Controls
- **Drive Status Pane:** 
    - Activity LED (Off, Green/Active, Red/Error).
    - Current Track/Sector display.
    - Error channel status (e.g., "00, OK, 00, 00").
- **CLI Commands:** 
    - `disk mount <unit> <file>`
    - `disk eject <unit>`
- **MCP Tools:** 
    - `mount_disk`
    - `eject_disk`
