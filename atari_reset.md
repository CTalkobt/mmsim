# Atari 400/800 Reset Sequence: From Reset Vector to BASIC Ready Prompt

This document provides a detailed technical walkthrough of the Atari 8-bit boot process, specifically focusing on the **OS Revision B (OSB)** ROM and **Atari BASIC**.

## Phase 1: The Reset Vector and Initial CPU State

Upon power-on or hardware reset, the 6502 CPU fetches the address stored at the Reset Vector ($FFFC-$FFFD). In the Atari OSB ROM, this vector points to **$E477**.

### Entry Routine: Cold Start Transition
Located at `$E477`, the OS provides a jump to the primary initialization logic.

```assembly
; Address: $E477 (OS Vector Table)
E477: JMP $F125    ; 4C 25 F1    Transition from the fixed vector table entry 
                   ;             to the actual start of the Cold Start logic.
```

---

## Phase 2: Cold Start Initialization ($F125)

The Cold Start routine is the "master" initialization sequence.

### Full Assembly Listing: Cold Start Entry

```assembly
; Address: $F125 (Start of Cold Start)
F125: SEI          ; 78          Disable maskable interrupts immediately to prevent 
                   ;             uninitialized handlers from firing during boot.
      LDA #$00     ; A9 00       Load zero into the accumulator.
      STA $08      ; 85 08       Set COLDST ($08) to 0. This flag tells the OS later 
                   ;             that a full hardware reset occurred, not a warm start.
      CLD          ; D8          Clear Decimal flag. The 6502 power-on state for this 
                   ;             flag is undefined; clearing it ensures standard binary math.
      LDX #$FF     ; A2 FF       Load $FF into X register.
      TXS          ; 9A          Set the Stack Pointer to $01FF. This defines the 
                   ;             top of the hardware stack in Page 1.
      JSR $F244    ; 20 44 F2    Call the memory sizing routine to find out how much 
                   ;             RAM is installed in the machine.
      JSR $F277    ; 20 77 F2    Call hardware init to silence audio, clear colors, 
                   ;             and reset the custom LSI chips.

      ; Clear Zero Page ($00-$FF)
      LDA #$00     ; A9 00       Prepare to clear memory with zeros.
      TAY          ; A0 00       Set index Y to 0.
F139: STA ($04),Y  ; 91 04       Use indirect-indexed addressing to clear ZP. 
                   ;             At this point, $04/$05 must point to $0000.
      INY          ; C8          Increment index.
      BNE F139     ; D0 F9       Repeat 256 times to clear the entire page.
      INC $05      ; E6 05       Move the high byte of the pointer to the next page.
      LDX $05      ; A6 05       Check current page being cleared.
      CPX $06      ; E4 06       Compare against the RAM limit found by sizing routine.
      BNE F139     ; D0 F1       Continue clearing until the end of available RAM.

      ; Initialize RAM vectors from ROM table at $E472
      LDA $E472    ; AD 72 E4    Read default DOS entry point (Low byte) from ROM.
      STA $0A      ; 85 0A       Store in DOSVEC RAM shadow.
      LDA $E473    ; AD 73 E4    Read default DOS entry point (High byte).
      STA $0B      ; 85 0B       Store in DOSVEC RAM shadow.

      ; Clear Pages 2 and 3 ($0200-$03FF)
      LDA #$FF     ; A9 FF       Prepare to set the COLDST shadow to $FF.
      STA $0244    ; 8D 44 02    $0244 is the RAM shadow of COLDST. $FF indicates 
                   ;             initialization is in progress.
      LDX #$00     ; A2 00       Reset X index.
F162: TXA          ; 8A          Clear A.
      STA $0200,X  ; 9D 00 02    Clear System Variables area (Page 2).
      STA $0300,X  ; 9D 00 03    Clear Cassette/Device buffer area (Page 3).
      DEX          ; CA          Decrement index.
      BNE F162     ; D0 F7       Repeat 256 times to clear both pages.
```

### Memory Sizing ($F244)
This routine determines how much RAM is present by testing the top of each 4K/16K block.

```assembly
; Address: $F244
F244: INC $BFFC    ; EE FC BF    Increment byte at top of 48K RAM limit. If it's RAM, 
                   ;             the value will change.
      LDA $BFFC    ; AD FC BF    Read it back.
      BNE F251     ; D0 08       If it changed (non-zero), we likely have RAM here.
      LDA $BFFD    ; AD FD BF    Check next byte for cartridge header presence.
      BPL F256     ; 10 03       If bit 7 is clear, no diagnostic cartridge found.
      JMP ($BFFE)  ; 6C FE BF    Jump to Diagnostic Cartridge entry if present.
F251: DEC $BFFC    ; CE FC BF    Restore the byte to its original state (0).
      LDY #$00     ; A0 00       Initialize Y for indexed access.
      STY $05      ; 84 05       Set pointer high byte to 0.
      LDA #$10     ; A9 10       Start RAM scan at $1000 (4K).
      STA $06      ; 85 06       Store scan start page.
F25C: LDA ($05),Y  ; B1 05       Read from the test address.
      EOR #$FF     ; 49 FF       Flip all bits in the accumulator.
      STA ($05),Y  ; 91 05       Attempt to write flipped bits to memory.
      CMP ($05),Y  ; D1 05       Read back and compare. If it's RAM, they will match.
      BNE F271     ; D0 DA       If they don't match, we've hit ROM or empty space.
      EOR #$FF     ; 49 FF       Restore original bits.
      STA ($05),Y  ; 91 05       Write original value back.
      LDA $06      ; A5 06       Load current page high byte.
      CLC          ; 18          Clear carry for addition.
      ADC #$10     ; 69 10       Increment by $10 pages (4K blocks).
      STA $06      ; 85 06       Update high byte.
      JMP $F25C    ; 4C 5C F2    Continue scan until failure.
F271: RTS          ; 60          Return to caller. $06 now contains the RAM limit.
```

### Hardware Initialization ($F277)
This routine clears all hardware registers to prevent "garbage" states.

```assembly
; Address: $F277
F277: LDA #$00     ; A9 00       Prepare zero value for clearing.
      LDX #$00     ; AA          Start index at 0.
F27A: STA $D000,X  ; 9D 00 D0    Clear GTIA registers ($D000-$D0FF). This resets 
                   ;             colors, PMG state, and collisions.
      STA $D400,X  ; 9D 00 D4    Clear ANTIC registers ($D400-$D4FF). This stops 
                   ;             DMA and clears NMI enables.
      STA $D200,X  ; 9D 00 D2    Clear POKEY registers ($D200-$D2FF). This silences 
                   ;             audio channels and resets timers.
      NOP          ; EA          Padding for timing/alignment.
      NOP          ; EA          Padding.
      NOP          ; EA          Padding.
      INX          ; E8          Increment register index.
      BNE F27A     ; D0 F1       Loop through all 256 possible register offsets.
      RTS          ; 60          Return to Cold Start.
```

---

## Phase 3: System Structure Setup

After hardware is ready, the OS initializes its internal data structures and copies vectors from `$E400` to RAM. This allows programs to "hook" system functions.

---

## Phase 4: Device and I/O Initialization

The OS calls initialization for the Editor (`E:`), Screen (`S:`), and Keyboard (`K:`). It sets up the default Display List (Gr. 0) and enables the cursor.

---

## Phase 5: The Boot Sequence

The OS checks for cartridges at `$A000` and `$8000`. If no cartridge is found, it attempts an SIO boot from Disk or Cassette. SIO boot involves loading a boot sector from the device into RAM and jumping to it.

---

## Phase 6: Handover to Atari BASIC

### Full Assembly Listing: BASIC Startup ($A000)

Atari BASIC (Rev B) starts with its own initialization to prepare the workspace for the user.

```assembly
; Address: $A000 (BASIC ROM Start)
A000: LDA $CA      ; A5 CA       Check the BASIC "Warm Start" flag.
      BNE A006     ; D0 04       If non-zero, BASIC was already running; skip cold init.
      LDA $08      ; A5 08       Check the OS Coldstart flag.
      BNE A04D     ; D0 45       If OS is warm, BASIC might need to perform a warm start.
      LDX #$FF     ; A2 FF       Load $FF.
      TXS          ; 9A          Reset the stack pointer specifically for BASIC.
      CLD          ; D8          Redundant clear of decimal flag for safety.
      LDX $02E7    ; AE E7 02    Get RAM limit from OS variable.
      LDY $02E8    ; AC E8 02    
      STX $80      ; 86 80       Set BASIC's internal MEMTOP ($80/$81). This is where 
                   ;             string/array space will end.
      STY $81      ; 84 81
      LDA #$00     ; A9 00       Load zero.
      STA $92      ; 85 92       Initialize internal BASIC variable (likely recursion depth).
      STA $CA      ; 85 CA       Mark BASIC as cold (uninitialized).
      INY          ; C8          (Looping logic starts to clear BASIC workspace)
      TXA          ; 8A
      LDX #$82     ; A2 82
A021: STA $00,X    ; 95 00       Clear Zero Page range specifically used by BASIC 
                   ;             for pointers and math.
      INX          ; E8
      STX $00      ; 94 00       Iterative clear logic.
      ...
```

### The READY Prompt
BASIC enters its main loop, waits for an input line, and if none is found, it displays the prompt.

```assembly
; Address: BASIC Main Loop Snippet
      LDA #$0A     ; A9 0A       Load default increment value (10).
      STA $C9      ; 85 C9       Set default line number increment for 'AUTO'.
      JSR $B8F1    ; 20 F1 B8    Call output routine to print a carriage return.
      JSR $BD45    ; 20 45 BD    Display prompt "READY". This routine pulls the 
                   ;             string from ROM and sends it to the CIO 'E:' device.
```

The string "READY" is stored in encoded form (with the 7th bit of the last character set as an end marker) within the BASIC ROM.

### Summary of Key Memory Locations
| Address | Name | Description |
|---|---|---|
| $FFFC | RESET | 6502 Reset Vector (Points to $E477) |
| $02E4 | RAMSIZ | Number of 256-byte pages of RAM found by OS |
| $0244 | COLDST | Coldstart flag (0 = Cold, initialized by OS) |
| $D000 | GTIA | Hardware: Graphics/Colors/Collisions |
| $D200 | POKEY | Hardware: Audio/Timers/Keyboard Scanning |
| $D300 | PIA | Hardware: Controller Ports/Memory Bank Switching |
| $D400 | ANTIC | Hardware: DMA Control and Display Lists |

Once the screen displays `READY`, the Atari has completed its reset sequence and is ready for user input.
