# mmemu-plugin-c64-pla: C64 PLA Banking Controller

The C64 PLA (Programmable Logic Array) is the hardware component responsible for memory banking on the Commodore 64. It is implemented as an `IOHandler` plugin within `mmemu-plugin-c64-pla.so`.

---

## 1. Intent

The C64 PLA decodes three control signals from the MOS 6510 I/O port (LORAM, HIRAM, CHAREN) and determines which combination of RAM, ROM, and I/O registers is visible in the upper address space on every memory read.

---

## 2. Functionality

### 2.1 Banking Regions

| Address Range | HIRAM | LORAM | CHAREN | Visible |
|---------------|-------|-------|--------|---------|
| $A000–$BFFF | 1 | 1 | — | BASIC ROM (8 KB) |
| $A000–$BFFF | 0 or 0 | — | — | Flat RAM |
| $D000–$DFFF | 1 | — | 0 | Character ROM (4 KB) |
| $D000–$DFFF | 1 | — | 1 | I/O devices (VIC-II, SID, CIA) |
| $D000–$DFFF | 0 | — | — | Flat RAM |
| $E000–$FFFF | 1 | — | — | KERNAL ROM (8 KB) |
| $E000–$FFFF | 0 | — | — | Flat RAM |

Signal logic: `1` = high (pulled up), `0` = low (driven).

### 2.2 Signal Sources

The three signals are `ISignalLine*` outputs from the `MOS6510` I/O port:

```cpp
pla->setSignals(cpu6510->signalLoram(),
                cpu6510->signalHiram(),
                cpu6510->signalCharen());
```

If a signal line is null (not connected), the PLA treats it as high (ROM-visible default).

### 2.3 ROM Injection

ROM images are injected by the machine factory after ROM loading:

```cpp
pla->setBasicRom (basicBuf,  8192);
pla->setKernalRom(kernalBuf, 8192);
pla->setCharRom  (charBuf,   4096);
```

Writes to ROM regions are never intercepted by the PLA — they fall through to the underlying flat RAM (matching real C64 hardware, where ROM is read-only but writes reach RAM behind it).

### 2.4 IORegistry Dispatch Order

The PLA registers with `baseAddr() = 0xA000`. The `IORegistry` sorts handlers by base address, so the PLA is called before the I/O device handlers at `$D000`. When the PLA returns `false` for a $D000–$DFFF access (CHAREN=1 or HIRAM=0), the IORegistry continues to VIC-II, SID, and CIA handlers — the correct behavior for I/O-visible state.

---

## 3. Dependencies

- **Depends on**: `ISignalLine` (from `mmemu-plugin-6502.so`, via `MOS6510`)
- **Required by**: C64 Machine Plugin (Phase 11.6)

---

## 4. Implementation Details

- **Source Location**: `src/plugins/devices/c64_pla/main/c64_pla.h/cpp`
- **Plugin**: `lib/mmemu-plugin-c64-pla.so`
- **Class**: `C64PLA : public IOHandler`
- **Registration Name**: `"c64pla"`
- **Base Address**: `0xA000` (for IORegistry sort ordering)
- **Address Mask**: `0xFFFF` (range checked internally per banking region)
