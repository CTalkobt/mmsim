; 45GS02 Advanced Instructions Test
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

    ; 1. Test INW / DEW
    lda #$FF
    sta $10
    lda #$00
    sta $11
    
    inw $10
    
    lda $10
    cmp #$00
    bne inw_fail
    lda $11
    cmp #$01
    bne inw_fail
    lda #$01
    sta RESULTS_BASE
    jmp test_dew
inw_fail:
    lda #$FF
    sta RESULTS_BASE

test_dew:
    ; 2. Test DEW
    lda #$00
    sta $10
    lda #$01
    sta $11
    
    dew $10
    
    lda $10
    cmp #$FF
    bne dew_fail
    lda $11
    cmp #$00
    bne dew_fail
    lda #$01
    sta RESULTS_BASE + 1
    jmp test_asl_a
dew_fail:
    lda #$FF
    sta RESULTS_BASE + 1

test_asl_a:
    ; 3. Test ASL A
    lda #$40
    asl A
    cmp #$80
    bne asl_fail
    lda #$01
    sta RESULTS_BASE + 2
    jmp test_lsr_a
asl_fail:
    lda #$FF
    sta RESULTS_BASE + 2

test_lsr_a:
    ; 4. Test LSR A
    lda #$80
    lsr A
    cmp #$40
    bne lsr_fail
    lda #$01
    sta RESULTS_BASE + 3
    jmp test_rol_a
lsr_fail:
    lda #$FF
    sta RESULTS_BASE + 3

test_rol_a:
    ; 5. Test ROL A with carry
    clc
    lda #$40
    rol A
    cmp #$80
    bne rol_fail
    lda #$01
    sta RESULTS_BASE + 4
    jmp end
rol_fail:
    lda #$FF
    sta RESULTS_BASE + 4

end:
    lda #$47
    sta MEGA65_KEY
    lda #$53
    sta MEGA65_KEY
    lda #$42
    sta EXIT_TRIGGER
    jmp end
