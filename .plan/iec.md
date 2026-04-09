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

## 6. Phase 15.2: Virtual IEC Device Implementation Plan

### 6.1 `VirtualIECBus` State Machine
The `VirtualIECBus` class will manage the high-level state of the serial bus protocol. It acts as a bridge between host-side disk images and the signal lines.

**States:**
- `IDLE`: Bus is inactive. All lines high (false).
- `ATTENTION`: Host has pulled ATN low. Devices are listening for address/command.
- `ADDRESSING`: Command byte is being received.
- `ACKNOWLEDGE`: Device has recognized its address and pulled DATA low.
- `READY_TO_RECEIVE`: Device is waiting for data from the Talker.
- `RECEIVING`: Bit-by-bit data transfer into internal buffer.
- `READY_TO_SEND`: Device has data and is waiting for the Listener.
- `SENDING`: Bit-by-bit data transfer from internal buffer.
- `ERROR`: Protocol violation or timeout.

**Internal Components:**
- `IECSignals`: Struct tracking the current state of ATN, CLK, DATA.
- `IECBuffer`: Byte buffer for current sector/file data.
- `FileHandler`: Interface for interacting with `.d64` or `.prg` images.

### 6.2 `IPortDevice` Integration (Level 2)
The `VirtualIECBus` will implement the `IPortDevice` interface to attach to CIA #2 (C64) or VIA #2 (VIC-20).

**C64 CIA #2 Port A ($DD00) Mapping:**
- **Bit 3 (Output):** ATN OUT.
- **Bit 4 (Output):** CLK OUT.
- **Bit 5 (Output):** DATA OUT.
- **Bit 6 (Input):** CLK IN (Inverse of bus CLK).
- **Bit 7 (Input):** DATA IN (Inverse of bus DATA).

**Implementation logic:**
- `writePort(val)`: Observes bits 3-5 to update the host's contribution to the bus.
- `readPort()`: Combines the host's outputs with the virtual device's outputs (wired-OR) and returns bits 6-7.
- `setDdr(ddr)`: Respects the Data Direction Register.

### 6.3 Handshake Timing and Ticking
Since the IEC bus is asynchronous but depends on timing (e.g., 1ms ATN delay, 60µs bit width), the `VirtualIECBus` must be ticked.
- `tick(uint64_t cycles)`: Updates the internal state machine based on elapsed cycles.
- **Sampling:** CLK and DATA lines are sampled periodically.
- **EOI detection:** Monitor the delay between CLK pulses.

### 6.4 Disk Image Streaming
- **Sector Read:** Map `TRACK/SECTOR` to image offset.
- **PRG Stream:** Read byte-by-byte from the host file and feed into the bit-shifter.
- **Status Channel:** Implement basic "00, OK, 00, 00" responses for command channel (secondary address 15).

### 6.5 Testing Strategy
- **Unit Test:** Mock `IPortDevice` calls and verify the state machine transitions through IDLE -> ATTENTION -> ACK -> DATA.
- **Integration Test:** Boot a C64 and use `LOAD "$",8` to verify the bit-level directory listing is received.
