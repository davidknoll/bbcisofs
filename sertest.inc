; Development/testing hack to allow connecting BeebEm to something that
; actually emulates an IDE CD-ROM drive, through the RS423 Destination /
; Remote Ethernet Port option.

; SHEILA addresses
ACIATDR = 0xFE09
ACIARDR = 0xFE09
ACIACTL = 0xFE08
ACIASTA = 0xFE09
SERPROC = 0xFE10

; Macros to take the place of simple LDA/STA instructions to FRED
        .macro iolda addr
        lda #0x11
        jsr seroutb
        lda #<addr
        jsr seroutb
        lda #>addr
        jsr seroutb
        jsr serinb
        .endm

        .macro iosta addr
        pha
        lda #0x00
        jsr seroutb
        lda #<addr
        jsr seroutb
        lda #>addr
        jsr seroutb
        pla
        jsr seroutb
        .endm

; Talk directly to the serial hardware
seroutb::
        pha
1$:     lda ACIASTA
        ror
        ror
        bcc 1$
        pla
        sta ACIATDR
        rts

serinb::
        lda ACIASTA
        ror
        bcc serinb
        lda ACIARDR
        rts
