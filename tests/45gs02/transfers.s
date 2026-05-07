; 45GS02 Register Transfer Test (Memory Results)
SERIAL_DATA = $d6c1
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

    ; Test TAZ / TZA
    lda #$55
    taz
    lda #$00
    tza
    cmp #$55
    beq z_ok
    lda #$FF
    sta RESULTS_BASE
    jmp test_b
z_ok:
    lda #$01
    sta RESULTS_BASE

test_b:
    ; Test TAB / TBA
    lda #$AA
    tab
    lda #$00
    tba
    cmp #$AA
    beq b_ok
    lda #$FF
    sta RESULTS_BASE + 1
    jmp end
b_ok:
    lda #$01
    sta RESULTS_BASE + 1

end:
    lda #$47
    sta MEGA65_KEY
    lda #$53
    sta MEGA65_KEY
    lda #$42
    sta EXIT_TRIGGER
    rts
