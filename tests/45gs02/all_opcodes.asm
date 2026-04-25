// KickAssembler 45GS02 Full Single-Byte Instruction Test
.cpu _45gs02

.const RESULTS = $0400
.const EXIT = $d6cf
.const SUCCESS = $55
.const MEGA65_KEY = $d02f

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

    // 1. CLE/SEE (E flag)
    clc
    cle
    php
    pla
    and #$20 // Bit 5 is E flag
    beq e_cle_ok
    lda #$E1
    sta RESULTS + 0
    jmp end
e_cle_ok:
    see
    php
    pla
    and #$20
    bne e_see_ok
    lda #$E2
    sta RESULTS + 0
    jmp end
e_see_ok:
    lda #SUCCESS
    sta RESULTS + 0

    // 2. TSY/TYS
    ldy #$55
    tys
    tsy
    cpy #$55
    beq tsy_ok
    lda #$E3
    sta RESULTS + 1
    jmp end
tsy_ok:
    lda #SUCCESS
    sta RESULTS + 1

    // 3. TSB/TRB (zp)
    lda #$00
    sta $20
    lda #$FF
    tsb $20
    lda $20
    cmp #$FF
    beq tsb_ok
    lda #$E4
    sta RESULTS + 2
    jmp end
tsb_ok:
    lda #$F0
    trb $20
    lda $20
    cmp #$0F
    beq trb_ok
    lda #$E5
    sta RESULTS + 2
    jmp end
trb_ok:
    lda #SUCCESS
    sta RESULTS + 2

    // 4. ASR A
    lda #$81
    asr
    cmp #$C0 // Sign bit preserved: 1100 0000
    beq asr_ok
    lda #$E6
    sta RESULTS + 3
    jmp end
asr_ok:
    lda #SUCCESS
    sta RESULTS + 3

    // 5. NEG A
    lda #5
    neg
    cmp #$FB
    beq neg_ok
    lda #$E7
    sta RESULTS + 4
    jmp end
neg_ok:
    lda #SUCCESS
    sta RESULTS + 4

    // 6. INZ / DEZ
    ldz #$10
    inz
    cpz #$11
    beq inz_ok
    lda #$E8
    sta RESULTS + 5
    jmp end
inz_ok:
    dez
    cpz #$10
    beq dez_ok
    lda #$E9
    sta RESULTS + 5
    jmp end
dez_ok:
    lda #SUCCESS
    sta RESULTS + 5

    // 7. PHZ / PLZ
    ldz #$AA
    phz
    ldz #$00
    plz
    cpz #$AA
    beq phz_ok
    lda #$EA
    sta RESULTS + 6
    jmp end
phz_ok:
    lda #SUCCESS
    sta RESULTS + 6

end:
    lda #$47        // Unlock MEGA65 I/O mode
    sta MEGA65_KEY
    lda #$53
    sta MEGA65_KEY
    lda #$42
    sta EXIT
    jmp start
