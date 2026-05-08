# IEC Protocol Bug Tracking

## Goal
Get `LOAD"$",8` working on the C64 emulator via the VirtualIEC bus device.

## Signal Model (CIA Pin Level)

The VirtualIEC operates at the CIA pin level, not the bus level:

### Write path ($DD00 output bits → writePort)
| Bit | Signal   | CIA pin = 1          | CIA pin = 0          |
|-----|----------|----------------------|----------------------|
| 3   | ATN OUT  | ATN asserted (bus LOW)  | ATN released (bus HIGH) |
| 4   | CLK OUT  | CLK asserted (bus LOW)  | CLK released (bus HIGH) |
| 5   | DATA OUT | DATA asserted (bus LOW) | DATA released (bus HIGH)|

The CIA output pins drive the bus through inverting open-collector drivers:
pin=1 → inverter → bus LOW (asserted). So `m_clkIn/m_dataIn/m_atnIn = 1` means "asserted".

### Read path (readPort → $DD00 input bits)
| Bit | Signal   | Bus LOW (asserted)     | Bus HIGH (released)    |
|-----|----------|------------------------|------------------------|
| 6   | CLK IN   | inverter → CIA reads 0  | inverter → CIA reads 1 |
| 7   | DATA IN  | inverter → CIA reads 0  | inverter → CIA reads 1 |

The bus lines pass through inverting buffers to CIA input pins:
bus LOW (asserted) → inverter → CIA pin = 0.

### Wired-OR bus
Either side (computer or device) can pull a line LOW (assert it). The bus is
released (HIGH) only when both sides release. This is open-collector/wired-OR:
`bus_asserted = computer_asserts || device_asserts`

### readPort implementation
```cpp
bool clkBus = m_clkIn || m_clkOut;   // true if anyone asserts CLK
bool dataBus = m_dataIn || m_dataOut; // true if anyone asserts DATA
// Invert for CIA input pins (bus asserted → CIA reads 0):
if (!clkBus) val |= (1 << 6);   // CLK released → bit 6 = 1
if (!dataBus) val |= (1 << 7);  // DATA released → bit 7 = 1
```

### handleBitTransfer
The KERNAL sender puts data on DATA line: bit=0 → assert DATA (m_dataIn=1),
bit=1 → release DATA (m_dataIn=0). So: `m_dataIn ? 0x00 : 0x80` is correct
(asserted = logic 0, released = logic 1).

### TALKING output
Device sets DATA for each bit: bit=0 → assert (m_dataOut=true),
bit=1 → release (m_dataOut=false). So: `m_dataOut = !((byte >> bit) & 1)`.

## Issues Found & Fixed

### 1. RETURN key not sent with `type` command
**Status: FIXED**
The `type` CLI command enqueues text but doesn't auto-append RETURN. Must use `\\n` escape (e.g. `type LOAD"$",8\n`).

### 2. Lowercase BASIC commands cause SYNTAX ERROR
**Status: FIXED**
`type load"$",8` types shifted keys (lowercase PETSCII). BASIC requires uppercase. Must use `type LOAD"$",8`.

### 3. DATA polarity inversion in receive path (handleBitTransfer)
**Status: FIXED**
`m_currentByte = (m_currentByte >> 1) | (m_dataIn ? 0x00 : 0x80)`.

### 4. DATA not held during bit transfer (ADDRESSING/LISTENING)
**Status: FIXED**
KERNAL checks DATA at each bit start — device must hold DATA asserted throughout 8-bit transfer, release only as byte ack.

### 5. EOI handshake missing in receive path
**Status: FIXED**
Added timeout detection (CLK released >250 cycles) and DATA pulse ack (phases 3-4).

### 6. DATA polarity inversion in transmit path (TALKING)
**Status: FIXED**
`m_dataOut = !((byte >> bit) & 1)` — assert DATA for 0-bit, release for 1-bit.

### 7. KERNAL HLE intercepting LOAD
**Status: NOT A BUG**
cbm-hle correctly checks getDiskStatus() and passes through to KERNAL when VirtualIEC handles the device.

### 8. readPort polarity inversion
**Status: FIXED**
readPort was returning asserted=1 on bits 6,7, but real hardware has inverting
input buffers: asserted (bus LOW) → CIA reads 0. Fixed by inverting the output:
`if (!clkBus) val |= (1 << 6)` instead of `if (clkBus)`.

### 9. ADDRESSING→LISTENING released DATA prematurely
**Status: FIXED**
When ADDRESSING detected ATN release and transitioned to LISTENING (or other
m_nextState), it released DATA (`m_dataOut = false`). The KERNAL checks DATA
immediately after releasing ATN — if DATA is released, it means "no device
present" and errors out. The KERNAL would skip sending the "$" filename byte
and go straight to UNLISTEN, leaving m_filename empty.

Fix: Removed `m_dataOut = false` from the ATN-release transition in ADDRESSING
(line 96). DATA now stays asserted (from the byte ack), so the KERNAL sees
"device present" and proceeds to send filename bytes in LISTENING mode.

**Result:** The "$" filename byte (0x24) is now correctly received in LISTENING.
The directory listing is generated, and the full TALK sequence begins.

### 10. TALK_READY phase 0→1 timing causes ACPTR to see premature CLK release
**Status: INVESTIGATING**
After the first byte is successfully sent (TALKING→TALK_FRAME→TALK_READY loop),
the second iteration of TALK_READY phase 0 waits 100 cycles then releases CLK.
But the C64's ACPTR enters and sees CLK already released — interpreting it as
an immediate "talker ready" or possibly EOI. The KERNAL then sets a status error
(ST bit 1 = read timeout), causing LOAD to report "FILE NOT FOUND".

**Observed behavior:** Screen shows `SEARCHING FOR $` then `?FILE NOT FOUND ERROR`.
The state machine reaches TALK_FRAME→TALK_READY for the second byte, but no more
state transitions occur after TALK_READY phase 0 completes (timer=100).

**Hypothesis:** After the device releases CLK (phase 0→1 transition), the C64's
ACPTR reads $DD00 and sees CLK already released. Depending on timing, ACPTR may:
(a) interpret this as immediate "ready to send" without proper setup, or
(b) enter EOI detection, causing a protocol mismatch.

The TALKING state uses 500-cycle CLK half-periods (500μs), which is ~10x slower
than real hardware (~20-60μs). ACPTR expects faster timing and may timeout.

**Key debug data:** After TALK_FRAME→TALK_READY (second byte), TALK_READY ph0
shows m_dataIn=1 consistently for 100 cycles. No writePort calls appear during
this window — the C64 hasn't written to $DD00 yet (still executing LOAD loop
code between ACPTR calls). After timer=100, CLK releases and no further state
transitions are logged (stuck in phase 1 or C64 aborted).

## Current State

Full ATN command sequence completes correctly:
LISTEN $28 → OPEN $F0 → "$" received in LISTENING → UNLISTEN $3F →
TALK $48 → SECOND $60 → TURNAROUND → TALK_READY → TALKING → TALK_FRAME

First byte of directory listing is sent successfully. TALK_FRAME→TALK_READY
loops back for the second byte, but the KERNAL aborts with FILE NOT FOUND
before the second byte completes.

Screen output: `SEARCHING FOR $` / `?FILE NOT FOUND ERROR` / `READY.`

## Debug Infrastructure
- `logDebug()` — verbose IEC messages at SIM_LOG_DEBUG level (writePort changes,
  TALK_READY phase 0 tick-by-tick)
- `log()` — important state transitions at SIM_LOG_INFO level
- CIA6526 has temp pre-address-check log for $DD00 ATN writes
- HLE has temp fprintf logs for vector hits
- json_machine_loader has temp IEC wiring debug
- State transitions logged via `log("State: %d -> %d")`

## Test Command
```
printf 'step 5000000\ntype LOAD\"$\",8\\n\nstep 200000000\nm 0400 07e8\nquit\n' | \
  timeout 600 bin/mmemu-cli -m c64 -i tests/resources/1541-demo.d64 2>debug.txt >output.txt
```

## Files Modified
- `src/plugins/devices/virtual_iec/main/virtual_iec.cpp` — IEC state machine, readPort fix, logDebug
- `src/plugins/devices/virtual_iec/main/virtual_iec.h` — added m_lastWriteVal, logDebug()
- `src/plugins/devices/cia6526/main/cia6526.cpp` — temp debug logging
- `src/plugins/cbm-hle/main/kernal_hle.cpp` — temp debug logging
- `src/libcore/main/json_machine_loader.cpp` — temp wiring debug logging
