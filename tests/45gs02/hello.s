; 45GS02 test - hello world
SERIAL_DATA = $d6c1
EXIT_TRIGGER = $d6cf
RESULTS_BASE = $0400

.org $2000

start:
    lda #$48
    sta RESULTS_BASE
    sta SERIAL_DATA
    lda #$45
    sta RESULTS_BASE + 1
    sta SERIAL_DATA
    lda #$4C
    sta RESULTS_BASE + 2
    sta SERIAL_DATA
    sta RESULTS_BASE + 3
    sta SERIAL_DATA
    lda #$4F
    sta RESULTS_BASE + 4
    sta SERIAL_DATA
    lda #10
    sta RESULTS_BASE + 5
    sta SERIAL_DATA

    lda #$42
    sta EXIT_TRIGGER
    jmp start
