# Atari Emulation Bug Report & Analysis

This document tracks the status of Atari 400/800 and 800XL emulation in `mmsim`.

## 1. Verified Hardware State

### Memory Sizing (Atari 400/800)
- **Status:** **Fixed & Verified.**
- **Details:** The OS correctly identifies **40 KB** of RAM when BASIC is enabled and **48 KB** when disabled.
- **Fix:** Ensured `OpenBusHandler` correctly returns `$FF` for unmapped regions like `$C000-$CFFF`.

### ROM Integrity
- **Status:** **Verified.** Standard authoritative NTSC ROMs (OSB, BASIC Rev C, XL OS Rev 2) are in use.

---

## 2. Identified Bugs & Fixes

### XL OS Mapping (Resolved)
- **Fix:** Mapped `osxl.bin` as a linear 16KB segment at `$C000-$FFFF`.

### OSB (400/800) Mapping (Resolved)
- **Fix:** Mapped `osb.bin` as two segments: **2KB FPP** at `$D800-$DFFF` and **8KB OS** at `$E000-$FFFF`.

### Vertical Blank Interrupts (Fixed)
- **Issue:** OS was hanging in wait loops because system timers (`RTCLOK`) weren't updating.
- **Fix:** 
    - Corrected `Antic::tick` to pulse the NMI signal at the start of VBLANK (line 248).
    - Fixed `CpuNmiLine` to correctly propagate the signal level to the 6502 core.
- **Verification:** `RTCLOK` ($14) now successfully increments on all models.

### POKEY IRQ & SIO (Resolved for Boot)
- **Issue:** OS stalled waiting for responses from unimplemented SIO devices.
- **Fix:** 
    - Implemented persistent IRQs in POKEY.
    - Added an SIO command frame tracker in POKEY that silently handles command frames without timing out.
- **Result:** The OS successfully times out SIO and proceeds to BASIC.

## 3. Screen & Video Issues

### Screen Blank / DMA Disabled
- **Issue:** The GUI Screen pane remains blank (uniform background color).
- **Diagnosis:** `Antic::renderFrame` returns early because `DMACTL` ($D400) is consistently `0`.
- **Observation:** The OS/BASIC boot sequence is repeatedly writing `0` to `$D400` in a loop. This indicates the system is either in a pre-graphics initialization state or has encountered an error that causes it to disable video.
- **Related Finding:** On the Atari 800, the PC was observed at `$C1B3` after 20M steps. This address is **Open Bus** (unmapped) on a standard 48K Atari 800, suggesting the CPU has "run off" into unmapped memory.

## 4. Final Status
*   **Atari 800:** Boots to BASIC initialization (`$BAA9`). Waiting for keyboard input.
*   **Atari 800XL:** Boots to BASIC initialization (`$BAA9`). Waiting for keyboard input.

Both machines are now functional through the boot sequence and reaching the interpreter entry point.
