        .title ISO-9660 (CDFS) for the BBC Micro
        .module osargs
        .r6500
        .area SWROM

; OSARGS handler
; Vector to be installed when we are the selected FS
; Entry:
;       A = opcode, X points to a control block in ZP,
;       Y = file handle or zero
; Exit:
;       A preserved except when called with A = Y = 0.
;       X, Y preserved.
argshndl::
        cpy #0          ; Call with or without file handle?
        bne 99$
        cmp #0          ; Return current filing system number
        bne 99$

        lda #0xCD
99$:    rts
