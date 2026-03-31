# mmemu-plugin-6502: MOS 6510 CPU Implementation

The MOS 6510 is a variant of the MOS 6502 found in the Commodore 64.  It is implemented as a subclass of `MOS6502` within `mmemu-plugin-6502.so`.

---

## 1. Intent

The 6510 extends the standard NMOS 6502 instruction set with a built-in 6-bit parallel I/O port at memory addresses $00 (DDR) and $01 (DATA).  On the C64 these port bits drive the PLA banking lines, controlling which combination of RAM, ROM, and I/O registers is visible in the upper address space.

---

## 2. Functionality

### 2.1 Built-in I/O Port

| Address | Register | Description |
|---------|----------|-------------|
| $0000   | DDR      | Data Direction Register — bit 0=output, bit 1=input; applies to bits 0–5 only |
| $0001   | DATA     | Port Data — drives the six output lines; input lines read back as 1 (pull-ups) |

Effective port level: `(DATA & DDR) | (~DDR & 0x3F)`

### 2.2 Port Bit Assignments

| Bit | Signal  | Purpose |
|-----|---------|---------|
| 0   | LORAM   | 0 = RAM at $A000–$BFFF; 1 = BASIC ROM visible (when HIRAM=1) |
| 1   | HIRAM   | 0 = RAM at $E000–$FFFF; 1 = KERNAL ROM visible |
| 2   | CHAREN  | 0 = Character ROM at $D000; 1 = I/O area visible |
| 3   | Cassette write output | Not used for banking |
| 4   | Cassette motor control | Not used for banking |
| 5   | Cassette sense input  | Not used for banking |

Power-on defaults: DDR = $00 (all inputs), DATA = $3F → all lines pulled high via hardware resistors, making KERNAL + BASIC + I/O visible immediately after reset.

### 2.3 ISignalLine Outputs

Three `ISignalLine*` accessors expose the current port output level to the C64 PLA:

```cpp
ISignalLine* signalLoram();   // bit 0
ISignalLine* signalHiram();   // bit 1
ISignalLine* signalCharen();  // bit 2
```

The machine factory passes these to `C64PLA::setSignals()` so the PLA can query them on every bus read.

### 2.4 Proxy Bus Implementation

Because `MOS6502::read` and `write` are private (non-virtual), the 6510 installs a thin `PortBus` proxy between the CPU core and the real system bus.  `PortBus` intercepts $0000/$0001 and forwards all other addresses unmodified.  The proxy is transparent to snapshot / write-log helpers, which are delegated directly to the real bus.

---

## 3. Instruction Set

The MOS 6510 executes the same NMOS 6502 instruction set (including all undocumented opcodes) with the same cycle counts and addressing modes.  See [README-6502.md](README-6502.md) for the full instruction reference.

---

## 4. Dependencies

- **Depends on**: `MOS6502` (same plugin)
- **Required by**: C64 machine plugin (Phase 11.6)

---

## 5. Implementation Details

- **Source Location**: `src/plugins/6502/main/cpu6510.h/cpp`
- **Plugin**: `lib/mmemu-plugin-6502.so` (bundled with the 6502 plugin)
- **Class**: `MOS6510 : public MOS6502`
- **Core Registration Name**: `"6510"`
- **Raw machine preset**: `"raw6510"` (64 KB flat RAM, no ROMs)
