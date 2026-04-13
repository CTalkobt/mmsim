# Commodore IEC Serial Bus Protocol Reference

This document describes the Commodore IEC (International Electrotechnical Commission) serial bus protocol as used by the C64, VIC-20, C128, Plus/4, and compatible peripherals. It is derived from the IEEE-488 parallel bus but reduced to a 5-wire serial implementation. Particular attention is given to state transitions and timing.

---

## 1. Electrical Layer

### 1.1 Connector (DIN 45322, 6-pin)

| Pin | Signal | Function                              |
|-----|--------|---------------------------------------|
| 1   | SRQ    | Service Request (active low, unused)  |
| 2   | GND    | Ground                                |
| 3   | ATN    | Attention — controller is commanding  |
| 4   | CLK    | Clock — controlled by current sender  |
| 5   | DATA   | Data — shared by all devices          |
| 6   | RESET  | System reset (active low)             |

### 1.2 Open-Collector Logic

All signal lines use open-collector (wired-OR) drivers with pull-up resistors (~1 kohm). Logic is **active low / active true**:

| Electrical | Logical | Meaning           |
|------------|---------|-------------------|
| 0 V        | True/1  | Line **pulled** (asserted) |
| +5 V       | False/0 | Line **released**          |

Any single device pulling a line low forces the bus-wide state to True, regardless of what other devices do. A line reads False only when **every** device has released it.

### 1.3 Host Hardware

**C64** — CIA #2 Port A (`$DD00`):

| Bit | Dir    | Signal    |
|-----|--------|-----------|
| 3   | Output | ATN OUT   |
| 4   | Output | CLK OUT   |
| 5   | Output | DATA OUT  |
| 6   | Input  | CLK IN    |
| 7   | Input  | DATA IN   |

**VIC-20** — VIA #2, same bit mapping.

---

## 2. Bus Roles

| Role       | Description |
|------------|-------------|
| Controller | Always the computer. Asserts ATN and sends command bytes. |
| Talker     | The single device currently transmitting data bytes. |
| Listener   | One or more devices currently receiving data bytes. |
| Passive    | A device not addressed; monitors ATN only. |

There can be **at most one talker** at a time. There must be **at least one listener** during a data transfer. The controller assigns roles via command bytes sent under ATN.

---

## 3. Command Byte Encoding

All command bytes are sent by the controller while ATN is asserted. Every device on the bus receives and decodes them.

### 3.1 Primary Commands

| Command   | Encoding          | Hex Range     | Function |
|-----------|-------------------|---------------|----------|
| LISTEN    | `001` + device₅   | `$20`–`$3E`  | Device becomes a listener |
| UNLISTEN  | `00111111`         | `$3F`         | All devices stop listening |
| TALK      | `010` + device₅   | `$40`–`$5E`  | Device becomes the talker |
| UNTALK    | `01011111`         | `$5F`         | All devices stop talking |

`device₅` is the 5-bit primary address (0–30). Address 31 is reserved for the global UNLISTEN / UNTALK commands.

A TALK command to device N implicitly un-talks any previously talking device.

### 3.2 Secondary Address Commands

| Command   | Encoding          | Hex Range     | Function |
|-----------|-------------------|---------------|----------|
| SECOND    | `011` + sa₅       | `$60`–`$7F`  | Select channel (reopen) |
| CLOSE     | `1110` + sa₄      | `$E0`–`$EF`  | Close channel |
| OPEN      | `1111` + sa₄      | `$F0`–`$FF`  | Open channel and associate with name |

`sa₅` is a 5-bit secondary address (0–31); `sa₄` is 4-bit (0–15). The secondary address selects a logical channel within the device (e.g., different files on a disk drive).

### 3.3 Typical Command Sequences

**LOAD "FILE",8,1:**
```
ATN: $28        LISTEN device 8
ATN: $F0        OPEN channel 0
      "FILE"    filename bytes (DATA mode, EOI on last)
ATN: $3F        UNLISTEN
ATN: $48        TALK device 8
ATN: $60        SECOND channel 0  (+ turnaround)
      ...       receive file data (DATA mode, EOI on last)
ATN: $5F        UNTALK
ATN: $28        LISTEN device 8
ATN: $E0        CLOSE channel 0
ATN: $3F        UNLISTEN
```

**SAVE "FILE",8:**
```
ATN: $28        LISTEN device 8
ATN: $F1        OPEN channel 1
      "FILE"    filename bytes (EOI on last)
ATN: $3F        UNLISTEN
ATN: $28        LISTEN device 8
ATN: $61        SECOND channel 1
      ...       send file data (EOI on last)
ATN: $3F        UNLISTEN
ATN: $28        LISTEN device 8
ATN: $E1        CLOSE channel 1
ATN: $3F        UNLISTEN
```

**Directory listing (`LOAD "$",8`):**
```
ATN: $28        LISTEN device 8
ATN: $F0        OPEN channel 0
      "$"       (single byte, with EOI)
ATN: $3F        UNLISTEN
ATN: $48        TALK device 8
ATN: $60        SECOND channel 0  (+ turnaround)
      ...       receive directory as BASIC program
ATN: $5F        UNTALK
ATN: $28        LISTEN device 8
ATN: $E0        CLOSE channel 0
ATN: $3F        UNLISTEN
```

---

## 4. Protocol State Transitions

The protocol has two major modes: **ATN mode** (command transfer) and **DATA mode** (payload transfer). Both use the same bit-level byte transfer mechanism on CLK/DATA, but ATN mode is framed by the ATN line and addresses all devices.

### 4.1 High-Level State Diagram

```
                          ATN asserted (any time)
                    ┌──────────────────────────────┐
                    │                              │
                    v                              │
              ┌──────────┐                         │
              │   IDLE   │                         │
              └────┬─────┘                         │
                   │ ATN asserted                  │
                   v                               │
              ┌──────────┐                         │
              │   ATN    │ all devices respond     │
              │ RESPONSE │ DATA=true within 1ms    │
              └────┬─────┘                         │
                   │ controller sends cmd bytes    │
                   v                               │
              ┌──────────┐                         │
              │ COMMAND  │ byte transfer under ATN │
              │ TRANSFER │ (one or more cmd bytes) │
              └────┬─────┘                         │
                   │ ATN released                  │
                   v                               │
         ┌────────┴────────┐
         │                 │
    ┌────v────┐      ┌─────v─────┐
    │  DATA   │      │ TURNAROUND│ (if TALK cmd was last)
    │TRANSFER │      │           │
    │(listen) │      └─────┬─────┘
    └────┬────┘            │
         │                 v
         │           ┌───────────┐
         │           │   DATA    │
         │           │ TRANSFER  │
         │           │  (talk)   │
         │           └─────┬─────┘
         │                 │
         └────────┬────────┘
                  │ EOI or ATN reasserted
                  v
              ┌──────────┐
              │   IDLE   │
              └──────────┘
```

### 4.2 ATN Mode Transitions (Detail)

ATN can be asserted at **any time**, even mid-byte. When asserted, all devices immediately abandon their current operation and prepare to receive commands.

```
State: IDLE
  Lines: CLK=released, DATA=released (or held by talker)
  Trigger: Controller asserts ATN

    ──► State: ATN RESPONSE
        Controller action: Pull ATN true, pull CLK true, release DATA
        Device action:     ALL devices pull DATA true within 1000 µs (Tat)
        Failure:           No DATA response → "DEVICE NOT PRESENT" (but
                           masked until actual data transfer attempted)
        Trigger: DATA goes true (device acknowledged)

    ──► State: COMMAND BYTE TRANSFER
        Uses standard byte transfer protocol (Section 5)
        All devices listen; each decodes whether it is addressed
        Multiple command bytes may be sent in sequence under ATN
        (e.g., LISTEN + OPEN, or TALK + SECOND)
        Trigger: ATN released by controller

    ──► State: POST-COMMAND
        If last command was TALK + SECOND → TURNAROUND (Section 6)
        If last command was LISTEN + SECOND/OPEN → DATA TRANSFER (talker=controller)
        If last command was UNLISTEN/UNTALK → IDLE
        If last command was CLOSE → IDLE (after UNLISTEN)
```

### 4.3 Data Transfer Transitions (Detail)

Data transfer occurs after ATN is released and roles have been established. The talker sends bytes to the listener(s) using the byte transfer protocol.

```
State: DATA TRANSFER (SENDING/RECEIVING)
  Trigger: Talker has data

    ──► State: BYTE TRANSFER (Section 5)
        Repeated for each byte
        Last byte signalled by EOI (Section 5.4)
        Trigger: EOI on last byte

    ──► State: IDLE
        Both talker and listener release all lines

  Trigger: Controller needs to send new command

    ──► State: ATN RESPONSE (controller asserts ATN)
        Transfer interrupted; all devices reset to command mode
```

---

## 5. Byte Transfer Protocol (Bit Level)

This is the core mechanism used in both ATN mode and DATA mode. The talker controls CLK; listeners use DATA for handshaking between bytes.

### 5.1 State Transition Diagram

```
                  ┌─────────────────────────────────────┐
                  │                                     │
     ┌────────────v──────────────┐                      │
     │    TALKER READY (Step 0)  │                      │
     │  Talker: CLK=true         │                      │
     │  Listener: DATA=true      │                      │
     └────────────┬──────────────┘                      │
                  │ Talker releases CLK                 │
                  v                                     │
     ┌───────────────────────────┐                      │
     │ READY TO SEND (Step 1)   │                      │
     │  Talker: CLK=released     │                      │
     │  Listener: DATA=true      │                      │
     │  (no time limit)          │                      │
     └────────────┬──────────────┘                      │
                  │ Listener releases DATA              │
                  v                                     │
     ┌───────────────────────────┐     >200 µs          │
     │ READY FOR DATA (Step 2)  │────────────┐         │
     │  Talker: CLK=released     │             │         │
     │  Listener: DATA=released  │             v         │
     │  Talker must respond      │    ┌──────────────┐  │
     │  within 200 µs (Tne)     │    │  EOI SIGNAL  │  │
     │  or it's EOI              │    │  (Section 5.4)│  │
     └────────────┬──────────────┘    └──────┬───────┘  │
                  │ Talker pulls CLK                    │
                  │ (within 200 µs = normal)            │
                  v                                     │
     ┌───────────────────────────┐                      │
     │  BIT TRANSFER (Step 3)   │                      │
     │  Repeat 8 times, LSB first│                      │
     │  See Section 5.2          │                      │
     └────────────┬──────────────┘                      │
                  │ All 8 bits sent                     │
                  v                                     │
     ┌───────────────────────────┐                      │
     │  FRAME HANDSHAKE (Step 4)│                      │
     │  Talker: CLK=true,        │                      │
     │          DATA=released    │                      │
     │  Listener must pull DATA  │                      │
     │  true within 1000 µs (Tf)│                      │
     └────────────┬──────────────┘                      │
                  │ Listener pulls DATA                 │
                  │ (acknowledges byte)                 │
                  │                                     │
                  └─────────────────────────────────────┘
                    Back to TALKER READY for next byte
                    (minimum 100 µs between bytes, Tbb)
```

### 5.2 Single Bit Transfer

Each of the 8 bits follows this sub-sequence:

```
  ┌─────────────────────────┐
  │ CLK=true (bit not valid) │  Talker places bit on DATA line
  │ Hold ≥ Ts (setup time)  │  Ts = 20 µs min (60 µs for C64)
  └───────────┬─────────────┘
              │ Talker releases CLK
              v
  ┌─────────────────────────┐
  │ CLK=released (data valid)│  Listener samples DATA on
  │ Hold ≥ Tv (valid time)  │  falling edge of CLK
  │ Tv = 20 µs min          │  (transition CLK true→false)
  │ (60 µs for C64)         │
  └───────────┬─────────────┘
              │ Talker pulls CLK true
              v
         Next bit (or frame handshake after bit 7)
```

Bits are transmitted **LSB first** (bit 0 through bit 7).

The listener samples DATA when CLK transitions from true to released (falling edge in electrical terms, i.e., rising voltage edge).

### 5.3 Timing Parameters

| Symbol | Parameter                    | Min     | Typ   | Max     | Notes |
|--------|------------------------------|---------|-------|---------|-------|
| Tat    | ATN response time            | —       | —     | 1000 µs | Device must pull DATA within this |
| Th     | Listener hold-off            | 0       | —     | ∞       | Listener may delay indefinitely |
| Tne    | Non-EOI response to RFD      | 40 µs   | —     | 200 µs  | Talker must begin within this or it's EOI |
| Ts     | Bit setup (CLK true hold)    | 20 µs   | —     | 70 µs   | C64 needs 60 µs due to VIC-II DMA |
| Tv     | Data valid (CLK released hold)| 20 µs  | —     | —       | C64 needs 60 µs |
| Tf     | Frame handshake              | 0       | —     | 1000 µs | Listener pulls DATA after last bit |
| Tr     | Frame to ATN release         | 20 µs   | —     | —       | After last cmd byte under ATN |
| Tbb    | Between bytes time           | 100 µs  | —     | —       | Talker waits after listener ACK |
| Tye    | EOI response time            | 200 µs  | —     | 250 µs  | Listener holds DATA to ACK EOI |
| Tei    | EOI response hold time       | 60 µs   | —     | —       | Minimum DATA hold for EOI ACK |
| Tda    | ATN acknowledge hold         | —       | —     | 1000 µs | Devices must respond to ATN |

### 5.4 EOI (End-Or-Identify) Signalling

EOI indicates the **last byte** of a transmission. It is signalled via a timing side-channel — the talker deliberately delays the start of the final byte.

```
State: READY FOR DATA (Step 2)
  Listener has released DATA
  Talker does NOT pull CLK within 200 µs (Tne)

    ──► State: EOI DETECTED
        Listener detects timeout (>200 µs with CLK still released)
        Listener acknowledges by pulling DATA true for ≥60 µs (Tei)
        Listener then releases DATA

    ──► State: EOI ACKNOWLEDGED
        Talker detects DATA pulled then released
        Talker proceeds with normal bit transfer (Step 3)
        This byte is marked as the final byte of the stream

    ──► After frame handshake (Step 4):
        Both sides know transfer is complete
        Talker and listener release all lines → IDLE
```

### 5.5 Empty Stream / File Not Found

If the talker has **no data at all** to send:

- Talker reaches Step 2 (READY FOR DATA) but never pulls CLK
- After 512+ µs, listener interprets this as an empty stream
- Listener may report "FILE NOT FOUND" or equivalent

---

## 6. Bus Turnaround (Role Reversal)

When the controller issues a TALK command, the device must become the talker and the controller must become a listener. This requires a **turnaround** — an orderly swap of CLK/DATA ownership.

### 6.1 Turnaround Sequence

This occurs immediately after ATN is released, when the last command under ATN was TALK + SECOND (or TALK + OPEN).

```
State: ATN RELEASED AFTER TALK
  Controller: was talker (CLK owner), now becoming listener
  Device:     was listener, now becoming talker

  Step 1: Controller releases CLK to false
          Controller pulls DATA true (signals "I am now listener")

    ──► Step 2: Device detects CLK released (false)
        Device pulls CLK true (takes over clock ownership)
        Device releases DATA

    ──► Step 3: TURNAROUND COMPLETE
        Device is now talker: CLK=true, DATA=released
        Controller is now listener: DATA=true (via controller)
        Normal data transfer proceeds (device sends to controller)
```

### 6.2 Turnaround State Diagram

```
  ┌──────────────────────────────┐
  │ PRE-TURNAROUND               │
  │ Controller: CLK=true          │
  │ Device:     DATA=true         │
  │ (end state of ATN command)   │
  └──────────────┬───────────────┘
                 │ ATN released
                 v
  ┌──────────────────────────────┐
  │ CONTROLLER YIELDS CLK        │
  │ Controller: CLK=released,     │
  │             DATA=true         │
  │ Device: waiting for CLK=false │
  └──────────────┬───────────────┘
                 │ Device sees CLK=released
                 v
  ┌──────────────────────────────┐
  │ DEVICE TAKES CLK              │
  │ Device: CLK=true,             │
  │         DATA=released         │
  │ Controller: DATA=true         │
  │ (= standard TALKER READY)    │
  └──────────────┬───────────────┘
                 │
                 v
          Normal data transfer
          (device is talker)
```

---

## 7. Complete Transaction Walkthrough

This section traces a complete LOAD operation through every state transition.

### 7.1 LOAD "FILE",8,1

**Phase 1: OPEN channel**

```
 IDLE
  │
  ├─ Controller asserts ATN, pulls CLK true, releases DATA
  │  → ATN RESPONSE: all devices pull DATA true (≤1000 µs)
  │
  ├─ Byte transfer: $28 (LISTEN device 8)
  │   Device 8 notes it is addressed as listener
  │
  ├─ Byte transfer: $F0 (OPEN channel 0)
  │   Device 8 prepares channel 0 for naming
  │
  ├─ Controller releases ATN
  │  → DATA TRANSFER mode (controller is talker, device 8 is listener)
  │
  ├─ Byte transfer: 'F' (no EOI)
  ├─ Byte transfer: 'I' (no EOI)
  ├─ Byte transfer: 'L' (no EOI)
  ├─ Byte transfer: 'E' (EOI — last byte of filename)
  │   Device 8 associates channel 0 with "FILE"
  │
  ├─ Controller asserts ATN
  │  → ATN RESPONSE
  ├─ Byte transfer: $3F (UNLISTEN)
  │  → All devices stop listening
  ├─ Controller releases ATN
  │  → IDLE
```

**Phase 2: Read data**

```
 IDLE
  │
  ├─ Controller asserts ATN
  │  → ATN RESPONSE
  │
  ├─ Byte transfer: $48 (TALK device 8)
  │   Device 8 notes it is addressed as talker
  │
  ├─ Byte transfer: $60 (SECOND channel 0)
  │
  ├─ Controller releases ATN
  │  → TURNAROUND (Section 6)
  │    Controller releases CLK, pulls DATA
  │    Device 8 pulls CLK, releases DATA
  │  → DATA TRANSFER mode (device 8 is talker, controller is listener)
  │
  ├─ Device 8 sends file bytes (byte transfer protocol)
  │   ... repeated for each byte ...
  ├─ Device 8 sends last byte with EOI
  │  → Controller acknowledges EOI
  │  → IDLE (both release lines)
  │
  │  (or controller may reassert ATN to send UNTALK)
  ├─ Controller asserts ATN
  ├─ Byte transfer: $5F (UNTALK)
  ├─ Controller releases ATN
  │  → IDLE
```

**Phase 3: CLOSE channel**

```
 IDLE
  │
  ├─ Controller asserts ATN
  ├─ Byte transfer: $28 (LISTEN device 8)
  ├─ Byte transfer: $E0 (CLOSE channel 0)
  ├─ Controller releases ATN
  │  → (device 8 closes channel 0, releases file resources)
  ├─ Controller asserts ATN
  ├─ Byte transfer: $3F (UNLISTEN)
  ├─ Controller releases ATN
  │  → IDLE
```

---

## 8. Error Conditions

| Condition | Detection | Timing | Result |
|-----------|-----------|--------|--------|
| Device not present | No DATA response at start of byte transfer | 256 µs after CLK released | Controller reports error |
| Listener not ready | DATA not released by listener | Indefinite (Th=∞) | Transfer stalls (not an error) |
| Frame handshake timeout | DATA not pulled after 8th bit | >1000 µs (Tf) | Talker signals error |
| EOI misdetection | CLK not pulled after DATA released | 200–512 µs | Listener interprets as EOI |
| Empty stream | CLK never pulled at all | >512 µs | Listener reports FILE NOT FOUND |
| ATN during transfer | ATN asserted mid-byte | Immediate | All devices abort, enter ATN RESPONSE |

---

## 9. Timing Diagram (Single Byte, Non-EOI)

```
         Th          Tne        ┌──── 8 bits (Ts+Tv each) ────┐    Tf
     ◄────────►  ◄─────────►   │                               │ ◄──────►
                                │                               │
CLK  ──┐       ┌──────────┐   ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐ ┌┐       ┌──────────
  true │       │ released  │   ││ ││ ││ ││ ││ ││ ││ ││  true  │
       └───────┘           └───┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘└────────┘

DATA ──────────────┐       ┌─d0┐d1┐d2┐d3┐d4┐d5┐d6┐d7─┐   ┌──────────
  true             │       │   bit values on DATA line │   │ listener
                   └───────┘                           └───┘  ACK

ATN  ─────────────────────────────────────────────────────────────────
  released (DATA mode) or true (ATN mode)

       [A]     [B]    [C]              [D]                [E]    [F]

  [A] Talker holds CLK true ("not ready to send")
  [B] Talker releases CLK ("ready to send")
  [C] Listener releases DATA ("ready for data") — talker responds <200 µs
  [D] 8 bits clocked out LSB-first, Ts+Tv per bit
  [E] Talker holds CLK true, releases DATA ("byte complete")
  [F] Listener pulls DATA true ("byte accepted") — within 1000 µs
```

---

## 10. Timing Diagram (EOI Byte)

```
         Th          >200 µs (EOI)    Tei        8 bits         Tf
     ◄────────►  ◄─────────────────► ◄────►
                                              (same as non-EOI)

CLK  ──┐       ┌─────────────────────────────┐ ┌┐ ┌┐ ...      ┌──────
  true │       │  released (talker delays)   │ ││ ││           │
       └───────┘                             └─┘└─┘└───...────┘

DATA ──────────────┐           ┌──────┐       d0 d1 ...        ┌──────
  true             │  listener │ ACK  │                        │
                   └───────────┘      └────────────...─────────┘
                   released    pulled  released
                               ≥60 µs

  1. Listener releases DATA ("ready for data")
  2. Talker does NOT pull CLK within 200 µs → EOI detected
  3. Listener pulls DATA true for ≥60 µs (EOI acknowledgement)
  4. Listener releases DATA
  5. Talker proceeds with normal 8-bit transfer
  6. After frame handshake, transfer is complete (no more bytes)
```

---

## 11. C64-Specific Notes

- The C64's VIC-II video chip steals CPU cycles for ~40 µs every ~500 µs (badline DMA). This forces minimum CLK hold times to 60 µs instead of the original 20 µs spec, roughly halving bus throughput to under 400 bytes/sec (~0.4 KB/s).
- The C64 has a 6526 CIA with a hardware shift register that could theoretically accelerate transfers, but it is not used because the 1541 drive's 6522 VIA has a known shift register bug.
- Fast-loaders (e.g., Epyx FastLoad, Action Replay) bypass the standard protocol entirely by bit-banging the bus lines with custom timing, achieving 2–10x speedups.

---

## 12. References

- Jim Butterfield, ["How the VIC/64 Serial Bus Works"](https://www.atarimagazines.com/compute/issue38/073_1_HOW_THE_VIC_64_SERIAL_BUS_WORKS.php), *Compute!* Issue 38, July 1983
- Michael Steil, ["Commodore Peripheral Bus: Part 4: Standard Serial"](https://www.pagetable.com/?p=1135), pagetable.com
- Michael Steil, ["Commodore Peripheral Bus: Part 2: Bus Arbitration, TALK/LISTEN"](https://www.pagetable.com/?p=1031), pagetable.com
- J. Derogee, ["IEC Disected"](https://janderogee.com/projects/1541-III/files/pdf/IEC_disected-IEC_1541_info.pdf), 2008
- [Commodore Bus](https://en.wikipedia.org/wiki/Commodore_bus), Wikipedia
- [Serial Port](https://www.c64-wiki.com/wiki/Serial_Port), C64-Wiki
