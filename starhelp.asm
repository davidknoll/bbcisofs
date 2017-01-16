        .title ISO-9660 (CDFS) for the BBC Micro
        .module starhelp
        .r6500
        .area SWROM

; *HELP handler
starhelp::
        pha
        jsr OSNEWL
        ldx #0x00
1$:     lda tlmsg,x
        beq 2$
        jsr OSWRCH
        inx
        jmp 1$
2$:     jsr OSNEWL
        pla
        rts
