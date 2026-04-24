// KickAssembler 45GS02 Register Transfer Test (Memory Results)
.cpu _45gs02

.const SERIAL_DATA = $d6c1
.const EXIT_TRIGGER = $d6cf
.const RESULTS_BASE = $0400

// BASIC header: 10 SYS 2064 ($0810)
* = $0801
    .byte $0C, $08, $0A, $00, $9E, $20, $32, $30, $36, $34, $00, $00, $00

* = $0810 "Program"

start:
    lda #0
    sta RESULTS_BASE
    sta RESULTS_BASE+1

    // Test TAZ / TZA
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
    // Test TAB / TBA
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
    lda #$42
    sta EXIT_TRIGGER
    jmp start
