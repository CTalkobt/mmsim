# mmemu Plugin: MOS 6520 PIA

**Plugin file:** `lib/mmemu-plugin-pia6520.so`
**Device ID:** `"6520"`
**Source:** `src/plugins/devices/pia6520/`

## Overview

The MOS 6520 Peripheral Interface Adapter (PIA) provides two independent 8-bit
bidirectional I/O ports (A and B), each with a dedicated data-direction register,
a pair of interrupt/handshake control lines (Cx1 input, Cx2 input/output), and a
dedicated IRQ output line. It is used in the Commodore PET (as PIA1 at $E810 and
PIA2 at $E820) and is functionally equivalent to the Motorola 6820.

## Register Map

Two address bits (`RS[1:0]`) select one of four register locations:

| RS | Address (PET PIA1) | Register | Access |
|----|-------------------|----------|--------|
| 00 | $E810 | ORA  | CRA bit 2 = 1 |
| 00 | $E810 | DDRA | CRA bit 2 = 0 |
| 01 | $E811 | CRA  | R/W (bits 7,6 read-only) |
| 10 | $E812 | ORB  | CRB bit 2 = 1 |
| 10 | $E812 | DDRB | CRB bit 2 = 0 |
| 11 | $E813 | CRB  | R/W (bits 7,6 read-only) |

`addrMask()` returns `0x0003`; only the two low address bits are decoded.

## Control Register (CRA / CRB) Bit Layout

| Bit | Name | Description |
|-----|------|-------------|
| 7 | IRQ1 flag | Set by Cx1 active edge; cleared by reading ORx. Read-only. |
| 6 | IRQ2 flag | Set by Cx2 active edge (input mode only); cleared by reading ORx unless independent mode. Read-only. |
| 5 | Cx2 dir | 0 = input, 1 = output |
| 4 | Cx2 ctrl B | See modes below |
| 3 | Cx2 ctrl A | See modes below |
| 2 | DDR/OR sel | 0 = DDR accessible at RS=x0; 1 = OR accessible |
| 1 | Cx1 IRQ en | 1 = active Cx1 edge asserts IRQx line |
| 0 | Cx1 edge | 0 = falling active, 1 = rising active |

## CA2 / CB2 Modes

When bit 5 = 0 (input):

| Bit 4 | Bit 3 | Mode |
|-------|-------|------|
| 0 | 0 | Input, falling edge triggers IRQ2 |
| 0 | 1 | Input, rising edge triggers IRQ2 |
| 1 | 0 | Input independent: IRQ2 not cleared by ORx read |
| 1 | 1 | Input independent + rising edge |

When bit 5 = 1 (output):

| Bit 4 | Bit 3 | Mode |
|-------|-------|------|
| 0 | 0 | Handshake: CA2 goes low on ORA read, released high by CA1 active edge |
| 0 | 1 | Pulse: CA2 pulses low on ORA read |
| 1 | 0 | Manual low |
| 1 | 1 | Manual high |

CB2 output is triggered by ORB **write** (not read); otherwise identical to CA2.

## IRQ Model

```
IRQx asserted when:
    (IRQ1_flag && Cx1_IRQ_EN) || (IRQ2_flag && Cx2_is_input)
```

- CA1 has an explicit interrupt enable (CRA bit 1). CA1 always detects edges and
  sets the IRQ1 flag; the line is only asserted if bit 1 = 1.
- CA2 in input mode always enables the IRQ2 contribution to IRQA.
- Reading ORA clears both flag bits (IRQ1 unconditionally; IRQ2 unless
  independent mode CRA bit 4 = 1).

## C++ API

```cpp
#include "pia6520.h"

PIA6520 pia("pia1", 0xE810);

// Wire peripheral devices
pia.setPortADevice(IPortDevice*);
pia.setPortBDevice(IPortDevice*);

// Wire signal lines
pia.setIrqALine(ISignalLine*);
pia.setIrqBLine(ISignalLine*);
pia.setCA1Line(ISignalLine*);
pia.setCA2Line(ISignalLine*);
pia.setCB1Line(ISignalLine*);
pia.setCB2Line(ISignalLine*);

// IOHandler interface (called by IO registry / bus)
pia.ioRead(bus, addr, &val);
pia.ioWrite(bus, addr, val);
pia.reset();
pia.tick(cycles);       // call each CPU cycle for edge detection

// Snapshot (save/restore state, e.g. for savestates)
PIA6520::Snapshot snap = pia.getSnapshot();
pia.restoreSnapshot(snap);
```

### IPortDevice

```cpp
class IPortDevice {
public:
    virtual uint8_t readPort()           = 0;  // read current pin levels
    virtual void    writePort(uint8_t v) = 0;  // drive output pins
    virtual void    setDdr(uint8_t d)    = 0;  // DDR changed
};
```

On ORA read: `(m_ora & m_ddra) | (pins & ~m_ddra)` — output bits from latch,
input bits from the attached device.

## Snapshot Struct

```cpp
struct Snapshot {
    uint8_t ora, ddra, cra;
    uint8_t orb, ddrb, crb;
    bool    ca1Prev, ca2Prev, cb1Prev, cb2Prev;
};
```

`restoreSnapshot()` rehydrates all registers and calls `updateIrqA()`/`updateIrqB()`
to reassert IRQ lines to match the restored state.

## Dependencies

- `libdevices` (`IOHandler`, `IPortDevice`, `ISignalLine`)
- No other plugins required

## Source Layout

```
src/plugins/devices/pia6520/
├── main/
│   ├── pia6520.h          — class declaration
│   ├── pia6520.cpp        — implementation
│   └── plugin_init.cpp    — registers "6520" factory
└── test/
    └── test_pia6520.cpp   — 10 unit tests
```
