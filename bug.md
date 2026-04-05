# Atari Boot Investigation (bug.md)

## Objective
The primary goal is to achieve a successful boot to the **Atari BASIC READY prompt** for the Atari 400 and Atari 800 (non-XL) machines.

## Problem Statement
The Atari machines are currently failing to display the BASIC prompt. Both the Atari 800 and 800XL models appear to hang or enter infinite loops during the final stages of OS and BASIC initialization.

## Current Issues & Findings

### 1. OS ROM Mapping (Resolved)
*   **Atari 800XL:** The 16KB `osxl.bin` is a linear dump. It is now correctly mapped as a single overlay at `$C000-$FFFF`, with I/O registers shadowing the `$D000-$D7FF` range.
*   **Atari 800 (OSB):** The 10KB `osb.bin` consists of the 2KB Floating Point Package (FPP) and the 8KB OS. It is now correctly split and mapped to `$D800` and `$E000` respectively.

### 2. Vertical Blank Interrupts (Fixed)
*   **Issue:** The Atari OS relies on the Vertical Blank Interrupt (VBI) from the ANTIC chip to update system timers (`RTCLOK`) and handle keyboard/SIO timeouts.
*   **Status:** Fixed logic in `Antic::tick` to pulse the NMI signal and manage the `NMIST` register bits correctly. `RTCLOK` ($14) is now successfully incrementing.

### 3. POKEY Interrupts & SIO (In Progress)
*   **Issue:** The OS stalls waiting for Serial I/O (SIO) operations to complete (checking `$3A` and `$0317`). Since SIO is currently a stub, these variables are never updated.
*   **Current Progress:** 
    *   Corrected POKEY IRQ logic to be persistent until cleared by writing to `IRQEN`.
    *   Implemented a basic SIO framework in POKEY that intercepts the 5-byte command frame and spoofs an **ACK ($41)** for Disk Drive 1 (D1:).
*   **Pending:** Verification that the OS correctly handles the spoofed ACK and proceeds past the `CHKTIM` routine.

### 4. Memory Sizing Validation
*   **Status:** Confirmed. The OS sizing routine at `$F244` correctly identifies 40KB of RAM when BASIC is enabled and 48KB when disabled. This confirms the memory bus and BASIC ROM boundaries are correctly configured.

## Next Steps
1.  Complete the final validation of the Atari 800 boot sequence using the `atari-debug` binary.
2.  Investigate why the Atari 800XL appears to be stuck in an IRQ handler or BASIC loop near `$B93C`.
3.  Stabilize the POKEY SIO state machine to ensure it reliably clears interrupts when `SERIN` is read.
