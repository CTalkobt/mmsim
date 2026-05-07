; 45GS02 Syntax Check Test
EXIT_TRIGGER = $d6cf
RESULTS_BASE = $0400

.org $2000

start:
    lda #$01
    sta RESULTS_BASE

    jsr label

    lda #$42
    sta EXIT_TRIGGER
    jmp start

label:
    rts
