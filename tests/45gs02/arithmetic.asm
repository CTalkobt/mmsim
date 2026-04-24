// KickAssembler 45GS02 Arithmetic Test (Memory Results)
.cpu _45gs02

.const SERIAL_DATA = $d6c1
.const EXIT_TRIGGER = $d6cf
.const RESULTS_BASE = $0400

// BASIC header: 10 SYS 2064 ($0810)
* = $0801
    .byte $0C, $08, $0A, $00, $9E, $20, $32, $30, $36, $34, $00, $00, $00

* = $0810 "Program"

start:
    // Initialize results memory to 0
    lda #0
    sta RESULTS_BASE
    sta RESULTS_BASE + 1

    // Test ADC: 1 + 1 = 2
    clc
    lda #1
    adc #1
    cmp #2
    beq adc1_ok
    lda #$FF
    sta RESULTS_BASE
    jmp test_sbc
adc1_ok:
    lda #$01
    sta RESULTS_BASE

test_sbc:
    // Test SBC: 5 - 3 = 2
    sec
    lda #5
    sbc #3
    cmp #2
    beq sbc1_ok
    lda #$FF
    sta RESULTS_BASE + 1
    jmp end
sbc1_ok:
    lda #$01
    sta RESULTS_BASE + 1

end:
    lda #10 // Newline for serial log still (optional)
    sta SERIAL_DATA
    lda #$42
    sta EXIT_TRIGGER
loop:
    jmp loop
