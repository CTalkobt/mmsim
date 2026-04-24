// KickAssembler 45GS02 test
.cpu _45gs02

.const SERIAL_DATA = $d6c1
.const EXIT_TRIGGER = $d6cf

* = $2000 "Program"

start:
    lda #'H'
    sta SERIAL_DATA
    lda #'E'
    sta SERIAL_DATA
    lda #'L'
    sta SERIAL_DATA
    sta SERIAL_DATA
    lda #'O'
    sta SERIAL_DATA
    lda #10 // Newline
    sta SERIAL_DATA

    lda #$42
    sta EXIT_TRIGGER
    jmp start // Should never hit this
