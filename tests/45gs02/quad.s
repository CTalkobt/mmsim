; 45GS02 Quad Instructions Test
EXIT_TRIGGER = $d6cf
MEGA65_KEY = $d02f
RESULTS_BASE = $0400

.org $2000

start:
    lda #$20
    ldx #0
loop_clear:
    sta RESULTS_BASE,x
    inx
    bne loop_clear

    ; 1. Test LDQ / STQ Zero Page
    lda #$11
    ldx #$22
    ldy #$33
    ldz #$44
    stq $10
    
    lda #0
    ldx #0
    ldy #0
    ldz #0
    
    ldq $10
    
    cpx #$22
    bne zp_fail
    cpy #$33
    bne zp_fail
    cpz #$44
    bne zp_fail
    cmp #$11
    bne zp_fail

    lda #$01
    sta RESULTS_BASE

    ; 2. Test LDQ / STQ Absolute
    lda #$55
    ldx #$66
    ldy #$77
    ldz #$88
    stq $1000
    
    lda #0
    ldx #0
    ldy #0
    ldz #0
    
    ldq $1000
    
    cpx #$66
    bne abs_fail
    cpy #$77
    bne abs_fail
    cpz #$88
    bne abs_fail
    cmp #$55
    bne abs_fail

    lda #$01
    sta RESULTS_BASE + 1

    ; 3. Test LDQ / STQ 32-bit Indirect [zp],z
    lda #$00
    sta $20
    lda #$20
    sta $21
    lda #$00
    sta $22
    lda #$00
    sta $23

    lda #$AA
    ldx #$BB
    ldy #$CC
    ldz #$DD
    
    ldz #0
    .byte $42, $42, $EA, $92, $20
    
    lda #0
    ldx #0
    ldy #0
    ldz #0
    
    ldz #0
    .byte $42, $42, $EA, $B2, $20
    
    cpx #$BB
    bne ind_fail
    cpy #$CC
    bne ind_fail
    cpz #$00          
    bne ind_fail
    cmp #$AA
    bne ind_fail

    lda #$01
    sta RESULTS_BASE + 2

    ; 4. Test ADCQ
    lda #$01
    ldx #$00
    ldy #$00
    ldz #$00
    stq $10
    
    lda #$FF
    ldx #$00
    ldy #$00
    ldz #$00
    clc
    adcq $10
    
    cpx #$01
    bne arith_fail
    cmp #$00
    bne arith_fail
    
    lda #$01
    sta RESULTS_BASE + 3

    ; 5. Test ASLQ
    lda #$00
    ldx #$00
    ldy #$00
    ldz #$80
    
    aslq
    
    bcc shift_fail
    cmp #$00
    bne shift_fail
    cpz #$00
    bne shift_fail

    lda #$01
    sta RESULTS_BASE + 4
    jmp end

zp_fail:
    lda #$FF
    sta RESULTS_BASE
    jmp end

abs_fail:
    lda #$FF
    sta RESULTS_BASE + 1
    jmp end

ind_fail:
    lda #$FF
    sta RESULTS_BASE + 2
    jmp end

arith_fail:
    lda #$FF
    sta RESULTS_BASE + 3
    jmp end

shift_fail:
    lda #$FF
    sta RESULTS_BASE + 4
    jmp end

end:
    lda #$47
    sta MEGA65_KEY
    lda #$53
    sta MEGA65_KEY
    lda #$42
    sta EXIT_TRIGGER
    rts
