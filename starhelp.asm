        .title ISO-9660 (CDFS) for the BBC Micro
        .module starhelp
        .r6500
        .area SWROM

; *HELP handler
starhelp::
        pha
        lda (0xF2),y    ; Is this an extended *HELP command?
        cmp #13
        bne 3$          ; We don't implement any yet
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
