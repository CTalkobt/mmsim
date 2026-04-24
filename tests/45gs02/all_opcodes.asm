// KickAssembler 45GS02 Full Instruction Test
.cpu _45gs02

.const RESULTS = $0400
.const EXIT = $d6cf

// BASIC header: 10 SYS 2064 ($0810)
* = $0801
    .byte $0C, $08, $0A, $00, $9E, $20, $32, $30, $36, $34, $00, $00, $00

* = $0810 "Program"

start:
    lda #0
    ldx #0
loop_clear:
    sta RESULTS,x
    inx
    bne loop_clear

    // 1. NEG test
    lda #5
    neg
    cmp #$FB // -5 in 2's complement
    beq neg_ok
    lda #$FF
    sta RESULTS + 0
    jmp end
neg_ok:
    lda #$01
    sta RESULTS + 0

    // 2. Accumulator Shifts (ASL, LSR, ROL, ROR)
    lda #$01
    asl
    cmp #$02
    beq asl_ok
    lda #$FF
    sta RESULTS + 1
    jmp end
asl_ok:
    lda #$01
    sta RESULTS + 1

    lda #$80
    lsr
    cmp #$40
    beq lsr_ok
    lda #$FF
    sta RESULTS + 2
    jmp end
lsr_ok:
    lda #$01
    sta RESULTS + 2

    // 3. Stack Relative (PHX/PLX, etc.)
    ldx #$42
    phx
    ldx #$00
    plx
    cpx #$42
    beq phx_ok
    lda #$FF
    sta RESULTS + 3
    jmp end
phx_ok:
    lda #$01
    sta RESULTS + 3

    // 4. MAP/EOM (Simple toggle)
    map
    eom
    lda #$01
    sta RESULTS + 4

end:
    lda #$42
    sta EXIT
    jmp start
