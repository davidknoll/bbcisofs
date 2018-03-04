        .title ISO-9660 (CDFS) for the BBC Micro
        .module starhelp
        .r6500
        .area SWROM

; *HELP handler
starhelp::
        pha
        lda [0xF2],y    ; Is this an extended *HELP command?
        cmp #0x0D
        bne 3$          ; Because we don't offer any yet

        jsr OSNEWL
        ldx #0x00       ; Output the ROM title string
1$:     lda tlmsg,x
        beq 2$
        jsr OSWRCH
        inx
        jmp 1$
2$:     jsr OSNEWL
3$:     pla
        rts
